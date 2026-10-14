//
//  main.cpp
//  TheBackTrackingSurveyPropagation
/*
 Copyright 2018 Raffaele Marino
 This file is part of BSP.

 BSP is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 BSP is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with BSP; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// IL PERDER TEMPO A CHI PIU' SA PIU' SPIACE

#define VERSION "BSP_SAT_VERSION_2018"
#include <bsp/Header.hpp>
#include <bsp/Vertex.hpp>
#include <bsp/Graph.hpp>
#include <bsp/run.hpp>
#include <bsp/thermo_sp.hpp>
#include <bsp/thermo_policy.hpp>
#include <bsp/tsallis.hpp>
#include <bsp/outdir.hpp>
#include <bsp/rsb2.hpp>
#include <bsp/sp2.hpp>
#include <cxxopts.hpp>
#define UNIX 1
#if UNIX
#define random() rand()
#define srandom(seed) srand(seed)
#endif

/*
This code describes message passing algorithms for solving random SAT problems.

At this moment the code is able to solve:

1) Random K-SAT problems with Survey Propagation, with backtracking strategy if needed, for any K.

This code is an open source code, it can be modified and improved with new strategies.
For any question about this code (and if you will find bugs), please, contact me at:
raffaele.marino@epfl.ch or marino.raffaele@mail.huji.ac.il or marinoraffaele.nunziatella@gmail.com 
 */




static void print_cli_help(const cxxopts::Options& options, const char* prog) {
    const char* name = (prog != NULL && prog[0] != '\0') ? prog : "bsp";
    cout << options.help() << "\n"
         << "Examples:\n"
         << "  " << name << " -w 3 4.0 50\n"
         << "  " << name << " --outdir=./myrun --seed=1 -w 3 4.0 50\n"
         << "  " << name << " -v --log-prefix -w 3 4.0 50\n"
         << "  " << name << " -l formula.cnf\n"
         << "\nWithout --outdir, all files go under bsp_runs/<K_N_a_s_scorer_...>/ with manifest.json.\n";
}

static int cli_main(int argc, char* argv[]) {
    const char* prog = (argv[0] != NULL && argv[0][0] != '\0') ? argv[0] : "bsp";

    cxxopts::Options options(prog,
        "Backtracking Survey Propagation for random K-SAT.\n"
        "  " + string(prog) + " [options] -w <K> <alpha> <N>\n"
        "  " + string(prog) + " [options] -l <formula.cnf>");
    options.custom_help("[options] (-w <K> <alpha> <N> | -l <formula.cnf>)");
    options.add_options()
        ("w,write", "Generate a random K-SAT instance (operands: K alpha N)")
        ("l,load", "Load a CNF formula from file and solve it",
            cxxopts::value<string>())
        ("h,help", "Show this help message")
        ("scorer", "Decimation scorer: cert|pol|i_c|fth|gamma:<g>|soft_cert:<beta>",
            cxxopts::value<string>())
        ("diag", "Write PREFIX_steps.csv / PREFIX_vars.csv diagnostics",
            cxxopts::value<string>())
        ("diag-every", "Log vars + residuals every K steps (default 1)",
            cxxopts::value<unsigned>())
        ("dump-residuals", "Also dump PREFIX_res_s<step>.cnf (needs --diag)")
        ("r", "Backtracking ratio in [0, 1) (default 0.9; 0 gives SID)",
            cxxopts::value<double>())
        ("seed", "Fix the RNG seed S >= 1 for reproducible runs",
            cxxopts::value<long>())
        ("theta", "Min direction margin |sT-sF|/(sT+sF) in [0, 1]",
            cxxopts::value<double>())
        ("veto", "Veto co-decimating vars sharing a clause")
        ("eps", "SP convergence threshold (default 0.01)",
            cxxopts::value<double>())
        ("damping", "SP update damping in [0, 1) (default 0)",
            cxxopts::value<double>())
        ("cav-temp", "Cavity temperature of ThermoSP (default 0)",
            cxxopts::value<double>())
        ("act-temp", "Gibbs action temperature (default 0; needs --cav-temp>0)",
            cxxopts::value<double>())
        ("rsb-m", "1RSB cluster reweighting exponent m (default 0); MMW closure, "
                  "adds m*ln2 to the unfrozen tilt gamma",
            cxxopts::value<double>())
        ("rsb-gamma", "1RSB unfrozen-cluster bias (default 0; not scorer gamma)",
            cxxopts::value<double>())
        ("tsallis-kappa", "Tsallis kappa=N(q-1): SP-y at y_eff=y/(1+kappa*y*e) "
                          "(default 0; needs --cav-temp>0)",
            cxxopts::value<double>())
        ("minisat", "Minisat binary (default minisat)",
            cxxopts::value<string>())
        ("oracle-timeout", "Per-check minisat seconds (default 10, 0=off)",
            cxxopts::value<unsigned>())
        ("oracle-dir", "Resolve each decimation direction by minisat")
        ("oracle-pick", "Scan top-K for a SAT-preserving move (0=off)",
            cxxopts::value<unsigned>())
        ("lookahead", "Top-K complexity lookahead (0=off)",
            cxxopts::value<unsigned>())
        ("corr-batch", "Prefer a distance-2 batch when batch size >= 2")
        ("adaptive-r", "Raise r after a steep Sigma drop or slow SP")
        ("dynamic-i-backtrack", "Release by smallest I(k) from current surveys")
        ("fe-backtrack", "Free-energy release order and Gibbs move split")
        ("bt-cost", "Cost of one back move in the Gibbs split",
            cxxopts::value<double>())
        ("frac", "Decimation batch fraction in (0, 1]",
            cxxopts::value<double>())
        ("rsb2-stab", "2RSB linear stability test: lambda in PREFIX_rsb2.csv"
            " (at mu1 = 1/cav-temp with --cav-temp)")
        ("rsb2-x", "2RSB population solver at x=mu1/mu2 in (0, 1] (default off)",
            cxxopts::value<double>())
        ("rsb2-pop", "2RSB population size per edge (default 64)",
            cxxopts::value<unsigned>())
        ("rsb2-every", "Run the 2RSB tools every K SP fixed points (default 1)",
            cxxopts::value<unsigned>())
        ("sp2-mu2", "Finite-mu 2RSB update at this mu2 (0 = off)",
            cxxopts::value<double>())
        ("sp2-x", "x = mu1/mu2 of --sp2-mu2 (default 0.5)",
            cxxopts::value<double>())
        ("sp2-pop", "Atoms per edge for --sp2-mu2 (default 32)",
            cxxopts::value<unsigned>())
        ("sp2-sweeps", "Sweeps of the finite-mu 2RSB update (default 4)",
            cxxopts::value<unsigned>())
        ("sp2-every", "Run --sp2-mu2 every K SP fixed points (0 = once)",
            cxxopts::value<unsigned>())
        ("outdir", "Directory for all file outputs (default: bsp_runs/<slug>)",
            cxxopts::value<string>())
        ("q,quiet", "Warnings and errors only")
        ("v,verbose", "Debug logging (-vv for trace)")
        ("log-level", "error|warn|info|debug|trace (default: info)",
            cxxopts::value<string>())
        ("log-prefix", "Prefix each line with [LEVEL]");

    /* cxxopts accepts -r but rejects single-char long options (--r=...); scripts use --r=. */
    vector<string> argv_store;
    argv_store.reserve(static_cast<size_t>(argc) + 4);
    for (int i = 0; i < argc; ++i) {
        const string a = argv[i] ? argv[i] : "";
        if (a.rfind("--r=", 0) == 0) {
            argv_store.push_back("-r");
            argv_store.push_back(a.substr(4));
        } else if (a == "--r") {
            argv_store.push_back("-r");
        } else {
            argv_store.push_back(a);
        }
    }
    vector<char*> argv_adj;
    argv_adj.reserve(argv_store.size());
    for (size_t i = 0; i < argv_store.size(); ++i)
        argv_adj.push_back(const_cast<char*>(argv_store[i].c_str()));
    int argc_adj = static_cast<int>(argv_adj.size());

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc_adj, argv_adj.data());
    } catch (const cxxopts::exceptions::exception& e) {
        BSP_ERROR << e.what() << endl;
        print_cli_help(options, prog);
        return 1;
    }

    if (result.count("help")) {
        print_cli_help(options, prog);
        return 0;
    }

    if (result.count("quiet"))
        bsp::set_log_level(bsp::LogLevel::Warn);
    else if (result.count("verbose") >= 2)
        bsp::set_log_level(bsp::LogLevel::Trace);
    else if (result.count("verbose") >= 1)
        bsp::set_log_level(bsp::LogLevel::Debug);
    if (result.count("log-prefix"))
        bsp::set_log_prefix(true);
    if (result.count("log-level")) {
        bsp::LogLevel level;
        if (!bsp::parse_log_level(result["log-level"].as<string>(), level)) {
            BSP_ERROR << "Unknown log level "
                      << result["log-level"].as<string>() << endl;
            print_cli_help(options, prog);
            return 1;
        }
        bsp::set_log_level(level);
    }

    if (result.count("scorer")) {
        if (!bsp_parse_scorer(result["scorer"].as<string>())) {
            BSP_ERROR << "Unknown scorer "
                      << result["scorer"].as<string>()
                      << " (expected cert|pol|i_c|fth|gamma:<g>|soft_cert:<beta>)" << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("diag"))
        g_diag_prefix = result["diag"].as<string>();
    if (result.count("dump-residuals"))
        g_dump_residuals = true;
    if (result.count("diag-every")) {
        g_diag_every = result["diag-every"].as<unsigned>();
        if (g_diag_every == 0) g_diag_every = 1;
    }
    if (result.count("r")) {
        g_r_bsp = result["r"].as<double>();
        if (!(g_r_bsp >= 0.0 && g_r_bsp < 1.0)) {
            BSP_ERROR << "r must be in [0, 1), got " << g_r_bsp << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("seed")) {
        g_fixed_seed = result["seed"].as<long>();
        if (g_fixed_seed < 1) {
            BSP_ERROR << "seed must be >= 1, got " << g_fixed_seed << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("theta")) {
        g_bsp_theta = result["theta"].as<double>();
        if (!(g_bsp_theta >= 0.0 && g_bsp_theta <= 1.0)) {
            BSP_ERROR << "theta must be in [0, 1], got " << g_bsp_theta << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("veto"))
        g_veto = true;
    if (result.count("eps")) {
        g_epsilon = result["eps"].as<double>();
        if (!(g_epsilon > 0.0)) {
            BSP_ERROR << "eps must be > 0, got " << g_epsilon << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("damping")) {
        g_damping = result["damping"].as<double>();
        if (!(g_damping >= 0.0 && g_damping < 1.0)) {
            BSP_ERROR << "damping must be in [0, 1), got " << g_damping << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("cav-temp")) {
        g_T_cav = result["cav-temp"].as<double>();
        if (!(g_T_cav >= 0.0)) {
            BSP_ERROR << "cav-temp must be >= 0, got " << g_T_cav << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("act-temp")) {
        g_T_act = result["act-temp"].as<double>();
        if (!(g_T_act >= 0.0)) {
            BSP_ERROR << "act-temp must be >= 0, got " << g_T_act << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("rsb-m"))
        g_rsb_m = result["rsb-m"].as<double>();
    if (result.count("rsb-gamma"))
        g_rsb_gamma = result["rsb-gamma"].as<double>();
    g_gamma_eff = g_rsb_gamma + g_rsb_m * M_LN2;
    if (result.count("tsallis-kappa"))
        g_tsallis_kappa = result["tsallis-kappa"].as<double>();
    if (result.count("minisat"))
        g_minisat_path = result["minisat"].as<string>();
    if (result.count("oracle-timeout"))
        g_oracle_timeout = result["oracle-timeout"].as<unsigned>();
    if (result.count("oracle-dir"))
        g_oracle_dir = true;
    if (result.count("oracle-pick"))
        g_oracle_pick = result["oracle-pick"].as<unsigned>();
    if (result.count("lookahead"))
        g_lookahead_k = result["lookahead"].as<unsigned>();
    if (result.count("corr-batch"))
        g_corr_batch = true;
    if (result.count("adaptive-r"))
        g_adaptive_r = true;
    if (result.count("dynamic-i-backtrack"))
        g_dynamic_i = true;
    if (result.count("fe-backtrack"))
        g_fe_backtrack = true;
    if (result.count("bt-cost"))
        g_bt_cost = result["bt-cost"].as<double>();
    if (result.count("frac")) {
        g_frac = result["frac"].as<double>();
        if (!(g_frac > 0.0 && g_frac <= 1.0)) {
            BSP_ERROR << "frac must be in (0, 1], got " << g_frac << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("rsb2-stab"))
        g_rsb2_stab = true;
    if (result.count("rsb2-x")) {
        g_rsb2_x = result["rsb2-x"].as<double>();
        if (!(g_rsb2_x > 0.0 && g_rsb2_x <= 1.0)) {
            BSP_ERROR << "rsb2-x must be in (0, 1], got " << g_rsb2_x << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("rsb2-pop")) {
        g_rsb2_pop = result["rsb2-pop"].as<unsigned>();
        if (g_rsb2_pop < 2) {
            BSP_ERROR << "rsb2-pop must be >= 2, got " << g_rsb2_pop << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("rsb2-every")) {
        g_rsb2_every = result["rsb2-every"].as<unsigned>();
        if (g_rsb2_every == 0) g_rsb2_every = 1;
    }
    if (result.count("sp2-mu2")) {
        g_sp2_mu2 = result["sp2-mu2"].as<double>();
        if (!(g_sp2_mu2 > 0.0)) {
            BSP_ERROR << "sp2-mu2 must be > 0, got " << g_sp2_mu2 << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("sp2-x")) {
        g_sp2_x = result["sp2-x"].as<double>();
        if (!(g_sp2_x > 0.0 && g_sp2_x <= 1.0)) {
            BSP_ERROR << "sp2-x must be in (0, 1], got " << g_sp2_x << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("sp2-pop")) {
        g_sp2_pop = result["sp2-pop"].as<unsigned>();
        if (g_sp2_pop < 2) {
            BSP_ERROR << "sp2-pop must be >= 2, got " << g_sp2_pop << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
    if (result.count("sp2-sweeps"))
        g_sp2_sweeps = result["sp2-sweeps"].as<unsigned>();
    if (result.count("sp2-every"))
        g_sp2_every = result["sp2-every"].as<unsigned>();
    if (g_sp2_mu2 > 0. && g_rsb2_x > 0.) {
        BSP_ERROR << "--sp2-mu2 does not combine with --rsb2-x" << endl;
        return 1;
    }

    const bool do_write = result.count("write") > 0;
    const bool do_load = result.count("load") > 0;
    const vector<string> unmatched = result.unmatched();

    if (do_write == do_load) {
        print_cli_help(options, prog);
        return 1;
    }
    if (do_write && unmatched.size() != 3) {
        BSP_ERROR << "-w expects exactly three operands: K alpha N" << endl;
        print_cli_help(options, prog);
        return 1;
    }
    if (do_load && !unmatched.empty()) {
        BSP_ERROR << "-l does not take extra operands beyond the CNF path" << endl;
        print_cli_help(options, prog);
        return 1;
    }
    if (g_dump_residuals && g_diag_prefix.empty()) {
        BSP_ERROR << "--dump-residuals requires --diag=PREFIX" << endl;
        return 1;
    }
    if ((g_scorer_id == 4 || g_T_act > 0.) && g_T_cav <= 0.) {
        BSP_ERROR << "--scorer=fth and --act-temp need --cav-temp>0"
                  << " (the free energies come from the deformed messages)" << endl;
        return 1;
    }
    if (g_tsallis_kappa != 0. && g_T_cav <= 0.) {
        BSP_ERROR << "--tsallis-kappa needs --cav-temp>0 (the base y is 1/cav-temp)" << endl;
        return 1;
    }
    if (g_T_cav > 0.)
        g_tsallis_y = 1. / g_T_cav;
    if (g_fe_backtrack && g_T_cav <= 0.) {
        BSP_ERROR << "--fe-backtrack needs --cav-temp>0"
                  << " (the release order runs on the free energies)" << endl;
        return 1;
    }
    if ((g_rsb2_stab || g_rsb2_x > 0.) && g_gamma_eff != 0.) {
        BSP_ERROR << "--rsb2-stab and --rsb2-x need SP or SP-y"
                  << " (no --rsb-m or --rsb-gamma)" << endl;
        return 1;
    }
    if (g_rsb2_x > 0. && g_T_cav > 0.) {
        BSP_ERROR << "--rsb2-x needs plain SP (no --cav-temp)" << endl;
        return 1;
    }
    if (g_oracle_pick > 0) {
        BSP_ERROR << "--oracle-pick is not implemented yet (use --oracle-dir)" << endl;
        return 1;
    }

    unsigned run_K = 0;
    double run_alpha = 0.;
    unsigned run_N = 0;
    string load_in;
    if (do_write) {
        run_K = static_cast<unsigned>(stoul(unmatched[0], nullptr, 0));
        run_alpha = stod(unmatched[1]);
        run_N = static_cast<unsigned>(stoul(unmatched[2], nullptr, 0));
    } else {
        load_in = result["load"].as<string>();
    }

    const bool have_outdir = result.count("outdir") > 0;
    const string outdir_arg = have_outdir ? result["outdir"].as<string>() : "";
    string load_abs, outdir_abs, cwd_before;
    if (bsp_enter_outdir(outdir_arg, have_outdir, run_K, run_alpha, run_N, load_in,
                         load_abs, outdir_abs, cwd_before) != 0)
        return 1;

    BspRunManifest man;
    man.outdir = outdir_abs;
    man.cwd_before = cwd_before;
    man.mode = do_write ? "write" : "load";
    man.load_path = load_abs;
    man.K = run_K;
    man.alpha = run_alpha;
    man.N = run_N;
    man.seed = g_fixed_seed;
    man.scorer = bsp_scorer_name();
    man.r = g_r_bsp;
    man.cav_temp = g_T_cav;
    man.act_temp = g_T_act;
    man.rsb_m = g_rsb_m;
    man.rsb_gamma = g_rsb_gamma;
    man.tsallis_kappa = g_tsallis_kappa;
    man.dynamic_i = g_dynamic_i;
    man.fe_backtrack = g_fe_backtrack;
    man.bt_cost = g_bt_cost;
    man.frac_batch = g_frac;
    man.eps = g_epsilon;
    man.damping = g_damping;
    man.theta = g_bsp_theta;
    man.diag = g_diag_prefix;
    man.diag_every = g_diag_every;
    man.rsb2_stab = g_rsb2_stab;
    man.rsb2_x = g_rsb2_x;
    man.rsb2_pop = g_rsb2_pop;
    man.rsb2_every = g_rsb2_every;
    man.timestamp_utc = bsp_utc_now();
    for (int i = 0; i < argc; ++i)
        man.argv.push_back(argv[i] ? argv[i] : "");
    bsp_write_manifest("manifest.json", man);

    vector<string> graph_args;
    graph_args.push_back(prog);
    if (do_write) {
        graph_args.push_back("-w");
        graph_args.push_back(unmatched[0]);
        graph_args.push_back(unmatched[1]);
        graph_args.push_back(unmatched[2]);
    } else {
        graph_args.push_back("-l");
        graph_args.push_back(load_abs);
    }

    vector<char*> av;
    av.reserve(graph_args.size());
    for (size_t i = 0; i < graph_args.size(); ++i)
        av.push_back(const_cast<char*>(graph_args[i].c_str()));
    int ac = static_cast<int>(av.size());

    Graph G(ac, &av[0]);/*declaration object Graph*/
    vector<Vertex> V;/*declaration vertex vector*/

    BSP_INFO<<"START ALGORITHM:"<<endl;

    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /***************************** START WRITE OR LOAD *********************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/

    /* At this point the code choses between loading a SAT CNF instance or building one. */
    if (do_load)
        G.read_from_file_graph(); /*read CNF instance from file*/
    else
        G.write_on_file_graph(); /*build and write a CNF instance on file*/
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /****************************** END WRITE OR LOAD **********************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/

    try {
        bsp::run_algorithm(G, V);
    } catch (const std::exception &e) {
        BSP_ERROR << e.what() << endl;
        return 1;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    try { return cli_main(argc,argv); }
    catch (const std::exception &e) { BSP_ERROR<<e.what()<<endl; return 1; }
}
