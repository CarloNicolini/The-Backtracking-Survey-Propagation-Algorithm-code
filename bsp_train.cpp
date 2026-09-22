#include "Header.h"
#include "NnScorer.hpp"
#include "third_party/genann/genann.h"
#include <cmath>
#include <cxxopts.hpp>
#include <sstream>

/*Train a 15-d -> hidden -> 1 MLP on PREFIX_dataset.csv rows produced by
 ./bsp --dataset=PREFIX. Writes a weights file loadable via --nn=FILE.
 With --eval FILE.nn: load weights, score rows, report ranking quality.*/

/*When false (default), rows with sat=-1 (no exact oracle label) are
 skipped: their DeltaSigma can look innocent on fatal moves.*/
static bool g_include_unknown = false;
/*With --target=sat, train y=sat in {0,1} (pure fatality classifier for
 veto mode) instead of DeltaSigma-with-FLOOR.*/
static bool g_target_sat = false;

struct Row {
    double x[BSP_NN_NFEAT];
    double y;
    int sat; /*1 SAT, 0 UNSAT, -1 unknown (oracle off / legacy row)*/
};

static bool parse_csv_line(const string& line, Row& r) {
    if (line.empty() || line[0] == '#') return false;
    if (line.compare(0, 4, "step") == 0) return false;
    vector<double> cols;
    string cell;
    istringstream in(line);
    while (getline(in, cell, ',')) {
        if (cell == "dec" || cell == "back") {
            cols.push_back(0.0);
            continue;
        }
        char* end = 0;
        cols.push_back(strtod(cell.c_str(), &end));
    }
    /*28-col legacy: ...,conv,crash,dS,eta2 (no oracle).
      29-col current: ...,conv,crash,sat,dS,eta2. Anything else is corrupt.*/
    if (cols.size() != 28 && cols.size() != 29) return false;
    int conv = static_cast<int>(cols[24] + 0.5);
    /*29-col file has sat_oracle at 26 and delta_sigma at 27; 28-col has delta at 26.*/
    size_t dcol = (cols.size() >= 29) ? 27 : 26;
    int sat = (cols.size() >= 29) ? static_cast<int>(cols[26]) : -1;
    if (sat != 0 && sat != 1) sat = -1;
    if (sat < 0 && !g_include_unknown) return false;
    if (sat < 0 && conv != 1) return false; /*no oracle: need convergence*/
    if (sat == 1 && conv != 1) return false; /*good move, unknown delta: skip*/
    /*sat==0 rows are kept regardless of conv; y is set to FLOOR in main*/
    double sT = cols[4], sF = cols[5], sI = cols[6];
    double bias = cols[7], abspol = cols[9], margin = cols[10];
    double n_inc = cols[14];
    double mean_len = 0.0;
    if (n_inc > 0.0)
        mean_len = (1.0 * cols[15] + 2.0 * cols[16] + 3.0 * cols[17] + 4.0 * cols[18]) / n_inc;
    r.x[0] = sT;
    r.x[1] = sF;
    r.x[2] = sI;
    r.x[3] = bias;
    r.x[4] = abspol;
    r.x[5] = margin;
    r.x[6] = cols[11];
    r.x[7] = cols[12];
    r.x[8] = cols[13];
    r.x[9] = n_inc;
    r.x[10] = mean_len;
    r.x[11] = cols[20];
    r.x[12] = cols[21];
    r.x[13] = cols[23];
    r.x[14] = cols[3]; /*dir*/
    r.y = cols[dcol];
    r.sat = sat;
    if (sat != 0 && !(r.y == r.y)) return false;
    return true;
}

static void print_cli_help(const cxxopts::Options& options) {
    cerr << options.help() << "\n"
         << "FILE.csv is PREFIX_dataset.csv from ./bsp --dataset=PREFIX.\n"
         << "Train: --data FILE.csv --out FILE.nn [--hidden N] [--epochs E] [--lr L]\n"
         << "Eval:  --eval FILE.nn --data FILE.csv\n";
}

int main(int argc, char** argv) {
    const char* prog = (argv[0] != NULL && argv[0][0] != '\0') ? argv[0] : "bsp-train";

    cxxopts::Options options(prog,
        "Train or evaluate a GenANN MLP on DeltaSigma dataset rows.");
    options.add_options()
        ("data", "PREFIX_dataset.csv from ./bsp --dataset=PREFIX",
            cxxopts::value<string>())
        ("out", "Output weights file for --nn=FILE",
            cxxopts::value<string>())
        ("eval", "Load weights and report ranking quality only",
            cxxopts::value<string>())
        ("hidden", "Hidden-layer width (default 16)",
            cxxopts::value<int>()->default_value("16"))
        ("epochs", "Training epochs (default 200)",
            cxxopts::value<int>()->default_value("200"))
        ("lr", "Learning rate (default 0.05)",
            cxxopts::value<double>()->default_value("0.05"))
        ("include-unknown",
            "Also train on rows without oracle labels (default: sat=0/1 only)")
        ("target", "sat|floor: y=sat classifier vs DeltaSigma+FLOOR (default floor)",
            cxxopts::value<string>()->default_value("floor"))
        ("h,help", "Show this help message");

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const cxxopts::exceptions::exception& e) {
        cerr << e.what() << endl;
        print_cli_help(options);
        return 1;
    }

    if (result.count("help")) {
        print_cli_help(options);
        return 0;
    }

    string data_path = result.count("data") ? result["data"].as<string>() : "";
    string out_path = result.count("out") ? result["out"].as<string>() : "";
    string eval_path = result.count("eval") ? result["eval"].as<string>() : "";
    int hidden = result["hidden"].as<int>();
    int epochs = result["epochs"].as<int>();
    double lr = result["lr"].as<double>();
    if (result.count("include-unknown"))
        g_include_unknown = true;
    {
        const string target = result["target"].as<string>();
        if (target == "sat")
            g_target_sat = true;
        else if (target == "floor")
            g_target_sat = false;
        else {
            cerr << "target must be sat or floor, got " << target << endl;
            print_cli_help(options);
            return 1;
        }
    }

    if (!eval_path.empty()) {
        if (data_path.empty()) { print_cli_help(options); return 1; }
        if (!bsp_nn_load(eval_path)) return 1;
        ifstream in(data_path.c_str());
        if (!in) {
            cerr << "Cannot open " << data_path << endl;
            return 1;
        }
        /*Ranking quality: P(pred_safe > pred_fatal) + means + abstention.*/
        vector<double> safe, fatal;
        string line;
        size_t n_abstain = 0, n_rows = 0;
        while (getline(in, line)) {
            Row r;
            if (!parse_csv_line(line, r)) continue;
            if (r.sat != 0 && r.sat != 1) continue;
            double y = 0.0;
            ++n_rows;
            if (!bsp_nn_predict(r.x, y)) { ++n_abstain; continue; }
            if (r.sat == 0) fatal.push_back(y);
            else safe.push_back(y);
        }
        if (safe.empty() || fatal.empty()) {
            cerr << "Need both classes, got safe=" << safe.size()
                 << " fatal=" << fatal.size() << endl;
            return 1;
        }
        double ms = 0.0, mf = 0.0;
        for (size_t i = 0; i < safe.size(); ++i) ms += safe[i];
        for (size_t i = 0; i < fatal.size(); ++i) mf += fatal[i];
        ms /= safe.size();
        mf /= fatal.size();
        sort(safe.begin(), safe.end());
        sort(fatal.begin(), fatal.end());
        double sq10 = safe[safe.size() / 10], sq50 = safe[safe.size() / 2];
        double fq50 = fatal[fatal.size() / 2], fq90 = fatal[fatal.size() * 9 / 10];
        /*AUC via sampling (full pairs too big): 200k random pairs.*/
        srand(7);
        size_t win = 0, tot = 200000;
        for (size_t t = 0; t < tot; ++t) {
            double a = safe[static_cast<size_t>(rand()) % safe.size()];
            double b = fatal[static_cast<size_t>(rand()) % fatal.size()];
            if (a > b) ++win;
            else if (a == b) win += 0;
        }
        cout << "rows=" << n_rows << " abstain=" << n_abstain
             << " mean_safe=" << ms << " mean_fatal=" << mf
             << " P(safe>fatal)=" << (static_cast<double>(win) / tot) << endl;
        cout << "safe_p10=" << sq10 << " safe_p50=" << sq50
             << " fatal_p50=" << fq50 << " fatal_p90=" << fq90 << endl;
        return 0;
    }
    if (data_path.empty() || out_path.empty() || hidden < 1 || epochs < 1 || lr <= 0.0) {
        print_cli_help(options);
        return 1;
    }

    ifstream in(data_path.c_str());
    if (!in) {
        cerr << "Cannot open " << data_path << endl;
        return 1;
    }
    vector<Row> rows;
    string line;
    while (getline(in, line)) {
        Row r;
        if (parse_csv_line(line, r)) rows.push_back(r);
    }
    /*Fatal trials (sat==0) get FLOOR: below every real delta, so ranking
     learns fatality first and DeltaSigma gradation second. With
     --target=sat, y=sat in {0,1} instead (pure fatality classifier).*/
    double floor = 1e300;
    size_t n_fatal = 0, n_sat = 0;
    if (g_target_sat) {
        vector<Row> keep;
        keep.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].sat == 0 || rows[i].sat == 1) {
                rows[i].y = rows[i].sat;
                keep.push_back(rows[i]);
            }
        }
        rows.swap(keep);
        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].sat == 0) ++n_fatal;
            else ++n_sat;
        }
        floor = 0.0;
    } else {
        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].sat == 0) ++n_fatal;
            else {
                ++n_sat;
                if (rows[i].y < floor) floor = rows[i].y;
            }
        }
        if (n_sat == 0) {
            cerr << "No non-fatal rows to train on" << endl;
            return 1;
        }
        floor -= 1.0;
        for (size_t i = 0; i < rows.size(); ++i)
            if (rows[i].sat == 0) rows[i].y = floor;
    }
    if (n_sat == 0 || n_fatal == 0) {
        cerr << "Need both classes, got sat=" << n_sat
             << " fatal=" << n_fatal << endl;
        return 1;
    }
    cout << "rows=" << rows.size() << " sat=" << n_sat
         << " fatal=" << n_fatal << " floor=" << floor << endl;
    if (rows.size() < 20) {
        cerr << "Need at least 20 usable rows, got " << rows.size() << endl;
        return 1;
    }

    size_t nval = rows.size() / 5;
    if (nval < 4) nval = 4;
    size_t ntrain = rows.size() - nval;
    srand(1);
    for (size_t i = 0; i < rows.size(); ++i) {
        size_t j = static_cast<size_t>(rand()) % rows.size();
        swap(rows[i], rows[j]);
    }

    double mean[BSP_NN_NFEAT], stdv[BSP_NN_NFEAT];
    for (int d = 0; d < BSP_NN_NFEAT; ++d) {
        mean[d] = 0.0;
        for (size_t i = 0; i < ntrain; ++i) mean[d] += rows[i].x[d];
        mean[d] /= static_cast<double>(ntrain);
        double v = 0.0;
        for (size_t i = 0; i < ntrain; ++i) {
            double z = rows[i].x[d] - mean[d];
            v += z * z;
        }
        v = sqrt(v / static_cast<double>(ntrain));
        stdv[d] = (v > 1e-12) ? v : 1.0;
    }

    genann* ann = genann_init(BSP_NN_NFEAT, 1, hidden, 1);
    if (!ann) {
        cerr << "genann_init failed" << endl;
        return 1;
    }
    ann->activation_output = genann_act_linear;

    double best_val = 1e300;
    int bad = 0;
    double cur_lr = lr;
    vector<size_t> order(ntrain);
    for (size_t i = 0; i < ntrain; ++i) order[i] = i;

    for (int ep = 0; ep < epochs; ++ep) {
        for (size_t i = 0; i < ntrain; ++i) {
            size_t j = static_cast<size_t>(rand()) % ntrain;
            swap(order[i], order[j]);
        }
        for (size_t t = 0; t < ntrain; ++t) {
            const Row& r = rows[order[t]];
            double z[BSP_NN_NFEAT];
            for (int d = 0; d < BSP_NN_NFEAT; ++d)
                z[d] = (r.x[d] - mean[d]) / stdv[d];
            genann_train(ann, z, &r.y, cur_lr);
        }
        double mse = 0.0;
        for (size_t i = ntrain; i < rows.size(); ++i) {
            double z[BSP_NN_NFEAT];
            for (int d = 0; d < BSP_NN_NFEAT; ++d)
                z[d] = (rows[i].x[d] - mean[d]) / stdv[d];
            double pred = genann_run(ann, z)[0];
            double e = pred - rows[i].y;
            mse += e * e;
        }
        mse /= static_cast<double>(nval);
        if (mse + 1e-12 < best_val) {
            best_val = mse;
            bad = 0;
        } else {
            ++bad;
            if (bad % 10 == 0) cur_lr *= 0.5;
        }
        if (ep % 20 == 0 || ep == epochs - 1)
            cout << "epoch " << ep << " val_mse=" << mse << " lr=" << cur_lr << endl;
        if (bad >= 25) {
            cout << "early stop at epoch " << ep << endl;
            break;
        }
    }

    FILE* f = fopen(out_path.c_str(), "w");
    if (!f) {
        cerr << "Cannot write " << out_path << endl;
        genann_free(ann);
        return 1;
    }
    fprintf(f, "# bsp-nn inputs=%d hidden=%d ntrain=%zu nval=%zu val_mse=%.8g\n",
            BSP_NN_NFEAT, hidden, ntrain, nval, best_val);
    fprintf(f, "# mean");
    for (int d = 0; d < BSP_NN_NFEAT; ++d) fprintf(f, " %.10g", mean[d]);
    fprintf(f, "\n# std");
    for (int d = 0; d < BSP_NN_NFEAT; ++d) fprintf(f, " %.10g", stdv[d]);
    fprintf(f, "\n");
    genann_write(ann, f);
    fclose(f);
    genann_free(ann);
    cout << "wrote " << out_path << " val_mse=" << best_val
         << " from " << rows.size() << " rows" << endl;
    return 0;
}
