//
//  release_check.cpp
//  BSP
//
//  Compares the release score -log I(k) with the change of G = Sigma_e - y E
//  after a real release (Prop. local of paper/sections/response.tex). Each
//  release runs in a forked child: release k, run SP to convergence, report G.
//
//  usage: bsp-release-check K alpha N seed n_dec n_trials [T_cav]
//  stdout: one CSV row per trial (dG is Sigma at T_cav=0), then a summary
//  line on stderr.
//  exit 1 if the new score has correlation < 0.9 with the measured change.
//

#include <bsp/Graph.hpp>
#include <bsp/Logger.hpp>
#include <bsp/thermo_sp.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

/*G = Sigma_e - y E is the functional of the local identity at finite y. At
 T_cav=0 it is Sigma.*/
static double potential(Graph &G) {
    return (g_T_cav > 0.) ? G.complexity - G.energy / g_T_cav : G.complexity;
}

/*G after releasing k, from a child process; NAN if SP failed there.*/
static double potential_after_release(Graph &G, Vertex *k) {
    int fd[2];
    if (pipe(fd) != 0) return NAN;
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        unsigned nfixed = G._list_fixed_element.size();
        swap(*find(G.ptrV.begin(), G.ptrV.begin() + nfixed, k), G.ptrV[nfixed - 1]);
        G._list_fixed_element.erase(k->_it_list_fixed_elem);
        k->_it_list_fixed_elem = G._list_fixed_element.end();
        k->reset_value_default_var_i();
        G.build(k);
        G.convergence_messages();
        G.surveys();
        double s = potential(G);
        ssize_t w = write(fd[1], &s, sizeof s);
        _exit(w == sizeof s ? 0 : 1);
    }
    close(fd[1]);
    double s = NAN;
    ssize_t r = read(fd[0], &s, sizeof s);
    close(fd[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    return (r == sizeof s && WIFEXITED(status) && WEXITSTATUS(status) == 0) ? s : NAN;
}

static double correlation(const vector<double> &x, const vector<double> &y) {
    double n = x.size(), mx = 0., my = 0., sxy = 0., sxx = 0., syy = 0.;
    for (size_t i = 0; i < x.size(); ++i) { mx += x[i] / n; my += y[i] / n; }
    for (size_t i = 0; i < x.size(); ++i) {
        sxy += (x[i] - mx) * (y[i] - my);
        sxx += (x[i] - mx) * (x[i] - mx);
        syy += (y[i] - my) * (y[i] - my);
    }
    return sxy / sqrt(sxx * syy);
}

int main(int argc, char **argv) {
    if (argc < 7) {
        fprintf(stderr, "usage: %s K alpha N seed n_dec n_trials [T_cav]\n", argv[0]);
        return 2;
    }
    unsigned n_dec = atoi(argv[5]), n_trials = atoi(argv[6]);
    g_fixed_seed = atol(argv[4]);
    if (argc > 7) g_T_cav = atof(argv[7]);
    bsp::set_log_level(bsp::LogLevel::Error);
    char tmpl[] = "/tmp/bsp_release_check_XXXXXX";
    if (!mkdtemp(tmpl) || chdir(tmpl) != 0) return 2;

    vector<string> args = {argv[0], "-w", argv[1], argv[2], argv[3]};
    vector<char *> av;
    for (string &a : args) av.push_back(&a[0]);
    Graph G((int)av.size(), &av[0]);
    G.write_on_file_graph();
    vector<Vertex> V(G.N());
    for (unsigned i = 0; i < G.N(); ++i) {
        V[i]._vertex = i + 1;
        V[i]._vertex_lli = (long int)(i + 1);
    }
    G.get_V(V);
    G.split_and_collect_information();
    G.unit_propagation();
    for (unsigned t = 0; t < n_dec; ++t) {
        G.convergence_messages();
        G.surveys();
        G.sort_V_Dec_move();
        G.choose_var_to_fix_and_clean();
    }
    G.convergence_messages();
    G.surveys();
    double sigma0 = potential(G);

    vector<Vertex *> fixed;
    for (Vertex *v : G._list_fixed_element)
        if (!v->_forced_by_up) fixed.push_back(v);
    if (fixed.size() > n_trials) fixed.resize(n_trials);

    vector<double> gain, pred_new, pred_old;
    printf("vertex,I_new,I_old,dG\n");
    for (Vertex *k : fixed) {
        double I_new = G.release_I(k, true), I_old = G.release_I(k, false);
        double s1 = potential_after_release(G, k);
        if (std::isnan(s1)) continue;
        printf("%u,%.12g,%.12g,%.12g\n", k->_vertex, I_new, I_old, s1 - sigma0);
        gain.push_back(s1 - sigma0);
        pred_new.push_back(-log(I_new));
        pred_old.push_back(-log(I_old));
    }
    double mae_new = 0., mae_old = 0.;
    for (size_t i = 0; i < gain.size(); ++i) {
        mae_new += fabs(pred_new[i] - gain[i]) / gain.size();
        mae_old += fabs(pred_old[i] - gain[i]) / gain.size();
    }
    double r_new = correlation(pred_new, gain), r_old = correlation(pred_old, gain);
    fprintf(stderr, "release-check: trials=%zu sigma0=%.6g corr_new=%.5f mae_new=%.3g corr_old=%.5f mae_old=%.3g\n",
            gain.size(), sigma0, r_new, mae_new, r_old, mae_old);
    return (gain.size() >= 10 && r_new >= 0.9) ? 0 : 1;
}
