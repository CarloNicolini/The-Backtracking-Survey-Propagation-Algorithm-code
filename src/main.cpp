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

//
//  Created by Raffaele Marino on 2018-08-25.
//  Copyright © 2018 Raffaele Marino. All rights reserved.
//


/*******************************************/
/*******************************************/
/*IL PERDER TEMPO A CHI PIU' SA PIU' SPIACE*/
/*******************************************/
/*******************************************/


#define MAIN
#define VERSION "BSP_SAT_VERSION_2018"
#include "Header.h"
#include "Vertex.hpp"
#include "Graph.hpp"
#include "NnScorer.hpp"
#include "thermo_sp.hpp"
#include "thermo_policy.hpp"
#include <cxxopts.hpp>
#define UNIX 1
#if UNIX
#define random() rand()
#define srandom(seed) srand(seed)
#endif

/*This code describes message passing algorithms for solving random SAT problems.



 ******************************************************************************************************************
 At this moment the code is able to solve:

    1) Random K-SAT problems with Survey Propagation, with backtracking strategy if needed, for any K.

 ******************************************************************************************************************


 This code is an open source code, it can be modified and improved with new strategies.
 For any question about this code (and if you will find bugs), please, contact me at:
 raffaele.marino@epfl.ch or marino.raffaele@mail.huji.ac.il or marinoraffaele.nunziatella@gmail.com */




static void print_cli_help(const cxxopts::Options& options, const char* prog) {
    const char* name = (prog != NULL && prog[0] != '\0') ? prog : "bsp";
    cout << options.help() << "\n"
         << "Examples:\n"
         << "  " << name << " -w 3 4.0 50\n"
         << "  " << name << " -v --log-prefix -w 3 4.0 50\n"
         << "  " << name << " -l formula.cnf\n";
}

int main(int argc, char* argv[]) {
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
        ("scorer", "Decimation scorer: cert|pol|i_c|fth|gamma:<g>",
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
        ("rsb-m", "1RSB cluster reweighting exponent m (default 0)",
            cxxopts::value<double>())
        ("dataset", "Write PREFIX_dataset.csv DeltaSigma trials",
            cxxopts::value<string>())
        ("dataset-k", "Shortlist size per step (default 50)",
            cxxopts::value<unsigned>())
        ("dataset-every", "Trial cadence in SP steps (default 1)",
            cxxopts::value<unsigned>())
        ("oracle", "Label each trial residual with minisat (on|off)",
            cxxopts::value<string>()->default_value("off")->implicit_value("on"))
        ("minisat", "Minisat binary (default minisat)",
            cxxopts::value<string>())
        ("oracle-timeout", "Per-trial minisat seconds (default 10, 0=off)",
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
        ("nn", "GenANN weights file from bsp-train",
            cxxopts::value<string>())
        ("nn-veto", "Veto mode: bury vars scoring below C (needs --nn)",
            cxxopts::value<double>())
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
                      << " (expected cert|pol|i_c|fth|gamma:<g>)" << endl;
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
    if (result.count("dataset"))
        g_dataset_prefix = result["dataset"].as<string>();
    if (result.count("dataset-k")) {
        g_dataset_k = result["dataset-k"].as<unsigned>();
        if (g_dataset_k == 0) g_dataset_k = 1;
    }
    if (result.count("dataset-every")) {
        g_dataset_every = result["dataset-every"].as<unsigned>();
        if (g_dataset_every == 0) g_dataset_every = 1;
    }
    if (result.count("oracle")) {
        const string o = result["oracle"].as<string>();
        if (o == "on")
            g_oracle = true;
        else if (o == "off")
            g_oracle = false;
        else {
            BSP_ERROR << "oracle must be on or off, got " << o << endl;
            print_cli_help(options, prog);
            return 1;
        }
    }
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
    if (result.count("nn"))
        g_nn_path = result["nn"].as<string>();
    if (result.count("nn-veto")) {
        g_nn_veto = true;
        g_nn_cutoff = result["nn-veto"].as<double>();
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
    if (g_fe_backtrack && g_T_cav <= 0.) {
        BSP_ERROR << "--fe-backtrack needs --cav-temp>0"
                  << " (the release order runs on the free energies)" << endl;
        return 1;
    }
    if (g_oracle_pick > 0) {
        BSP_ERROR << "--oracle-pick is not implemented yet (use --oracle-dir)" << endl;
        return 1;
    }
    if (!g_nn_path.empty()) {
        if (!bsp_nn_load(g_nn_path)) return 1;
    }

    vector<string> graph_args;
    graph_args.push_back(prog);
    if (do_write) {
        graph_args.push_back("-w");
        graph_args.push_back(unmatched[0]);
        graph_args.push_back(unmatched[1]);
        graph_args.push_back(unmatched[2]);
    } else {
        graph_args.push_back("-l");
        graph_args.push_back(result["load"].as<string>());
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

    /*At this point the code initializes the vertex Vector. */
    V.resize(G.N());/*Initialization Vector V*/
    for (unsigned int i=0; i<G.N(); ++i) {
        V[i]._vertex=i+1; /*label each variable node with a number form 1 to N*/
        V[i]._vertex_lli=(long int)(i+1);
    }

    /*In graph G the vector V is stored in a vector of pointers*/
    G.get_V(V);/*pass vector V to object G*/

    /*An instance of SAT, K-SAT, or NAESAT, is given as a conjunction of one or more clauses,
     where a clause is a disjunction of literals. For the XORSAT problem, instead, a clause takes the form of
     a "XOR" function of literals. Each instance can be represented by a bipartite graph, called factor graph.
     Each clause corresponds to a function node, each variable to a variable node,
     and an edge connects a function node and a variable node if and only if the clause contains the variable.
     All these information are collected in split_and_collect_information(). This public member of class Graph
     helps to split a CNF instance into Vertex objects and Clause objects.
     These objects store all types of information about the factor graph.
     The reader can see the specific for each class member in Graph.hpp, Vertex.hpp and Graph.cpp,
     Vertex.cpp files.*/

    G.split_and_collect_information();/*split and collect information into graph G*/
    if(g_r_bsp!=0.)BSP_INFO<<"START BSP WITH r="<<g_r_bsp<<":"<<endl;
    else BSP_INFO<<"START SID:"<<endl;
    BSP_INFO<<"Decimation scorer: "<<bsp_scorer_name()<<endl;
    if (g_T_cav!=0.)BSP_INFO<<"ThermoSP cavity temperature: T="<<g_T_cav<<endl;
    if (g_T_act!=0.)BSP_INFO<<"Gibbs action temperature: T_act="<<g_T_act<<endl;
    if (g_rsb_m!=0.)BSP_INFO<<"1RSB cluster reweighting: m="<<g_rsb_m<<endl;
    if (g_fe_backtrack)BSP_INFO<<"Free-energy backtracking on, bt-cost="<<g_bt_cost<<endl;
    if (g_lookahead_k>1 || g_corr_batch || g_adaptive_r || g_frac!=frac || g_dynamic_i)
        BSP_INFO<<"profile controls: lookahead="<<g_lookahead_k
                <<" corr_batch="<<(g_corr_batch?1:0)
                <<" adaptive_r="<<(g_adaptive_r?1:0)
                <<" frac="<<g_frac
                <<" dynamic_i="<<(g_dynamic_i?1:0)<<endl;
    /*Check if we have to use unit propagation*/
    G.unit_propagation();

    goto SP;
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /********************************* START BSP ***************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/


    /*The heart of message passing procedures are iterative equations.
     Survey propagation (SP) algorithm is based on equations  derived by cavity method.

     For cavity method and analytic formulation of SP equations we refer to :

     [1] Marino, Raffaele, Giorgio Parisi, and Federico Ricci-Tersenghi. "The backtracking survey propagation algorithm for solving random K-SAT problems." Nature communications 7 (2016): 12996.
     [2] Mézard, Marc, Giorgio Parisi, and Riccardo Zecchina. "Analytic and algorithmic solution of random satisfiability problems." Science 297.5582 (2002): 812-815.
     [3] Braunstein, Alfredo, and Riccardo Zecchina. "Survey propagation as local equilibrium equations." Journal of Statistical Mechanics: Theory and Experiment 2004.06 (2004): P06007.
     [4] Mézard, Marc, and Giorgio Parisi. "The cavity method at zero temperature." Journal of Statistical Physics 111.1-2 (2003): 1-34.
     [5] Mézard, Marc, and Riccardo Zecchina. "Random k-satisfiability problem: From an analytic solution to an efficient algorithm." Physical Review E 66.5 (2002): 056126.
     [6] Parisi, Giorgio. "On the survey-propagation equations for the random K-satisfiability problem." arXiv preprint cs/0212009 (2002).
     [7] Parisi, Giorgio. "A backtracking survey propagation algorithm for K-satisfiability." arXiv preprint cond-mat/0308510 (2003).
     [8] Aurell, Erik, Uri Gordon, and Scott Kirkpatrick. "Comparing beliefs, surveys, and random walks." Advances in Neural Information Processing Systems (2005).


     SP equations return messages that go from each caluse to each variable node,
     and compute for each variable node surveys. A message sent from a clause c to a variable i
     informs the variable what is the best choice to do for satisfying clause c. This message is computed
     from the messages recived by remaining variables j into clause c, but distinct from i.
     All messages are computed iteratively till a fixed point shows up.
     In this code this procedure is obtained by the public member of class Graph convergence_messages().
     For details, we refer to technical specifications into class Graph.hpp and .cpp .
     Once a convergece is found, surveys for each variable node are computed. They are three,
     and each of them describes the probability that a variable node can be assigned to
     a true value, s_T, to a false value, s_F, or can be indeterminate s_I.
     This calculation, in this code, is performed by the public member surveys().
     For details, we refer to technical specifications in class Graph and class Vertex.
     Moreover, we also compute the certitude, defined as:

                                                s_C=(1.-min(s_T,s_F).

     This simple bias will help us to decide which variable(s) can be fixed or relased
     during decimation or backtracking procedure.*/
SP:

    G.convergence_messages();/*find messages convergence*/
    G.surveys();/*compute surveys for variable nodes*/
    G.diag_step();/*log SP fixed point (no-op unless --diag)*/
    G.dataset_trials();/*tentative-fix trials (no-op unless --dataset)*/
    G.apply_nn_scores();/*overwrite scores if --nn=weights was given*/

    /*The backtracking survey propagation (BSP) algorithm proceeds similarly to survey inspired decimation (SID), by alternating decimation or backtracking steps on a fraction f of variables, in order to keep the algorithm efficient. The choice between a decimation or a backtracking step is taken accordingly to a stochastic rule, where the parameter r ∈ [0,1) represents the ratio between backtracking steps to decimation steps [1]. When r=0 one obtains survey inspired decimation (SID), while when r!=0 one works with backtracking survey propagation. In this code r=_R_BSP into Header.h file.
     */
    if (!G.fl_bsp) {    /*choice between backtracking strategy or decimation  strategy*/

        /*decimation strategy*/

        /* The main step in decimation procedure consists in starting from a problem with N variables and assign
         a variable node to reduce the order of the factor graph and its size, with the hope to make it simpler.
         This strategy proceeds in the following way. One chooses a variable node i with the largest value of
         certitude s_C, assigns this variable to true or false using the rule:

                                             i is TRUE if s_T>s_F;
                                             i if FALSE if s_F>s_T,

         and removes it from the factor graph. Then one removes, in according to the problem
         (i.e. if is a SAT, XORSAT or NAESAT), all the clauses that are satisfied by the assignment done, and in all
         cluases not sitisfied by the assignment of i, the literal associated is removed.
         In a faster version one can decimate L variable nodes with maximal s_C, where L = fN with f a small number,
         instead of decimate only one variable node each time. If one is not too near the critical point and f < .01
         the results are only weakly dependent on f [3,7].
         This procedure is described by two public members in class Graph, i.e. sort_V_Dec_move() and
         choose_var_to_fix_and_clean(). sort_V_Dec_move() helps us to pick up variable nodes with the highest value
         of certitude. It is a sort and the computational complexity is O(NlogN). choose_var_to_fix_and_clean()
         fixes variable nodes and cleans the graph, as described above. It has a computational complexity O(logN).
         Onece decimation step is terminated a new convergence of clause to variable messages is needed.
         This strategy proceeds iteratively till one of these conditions appears:

                 1) all messages computed by SP equations are trivial, i.e. all of them are 0. In this case one is arrived to an "easy" region, called by physicist paramagnetic phase, where the residual graph can be solved using greedy or heuristic (polynomial)algorithms.
                 2) SP equations does not find any fixed point. In this case we claim that the algorithm fails.
         */
        BSP_INFO<<G;/*print on terminal complexity*/
        G.save();
        if(G.complexity==0)goto PARAPHASE; /*go to PARAPHASE for building final solution*/
        if(G.complexity<-2.) {
            /*By experience we know that when N is large, e.g. (> 1000), and SP
             gives a negative complexity, with probability equal to one SP will not converge. For this
             reason we have set an exit strategy here.*/
            /*This point should be analyzed carefully from the scientific community*/
            BSP_ERROR<<"Negative complexity: I am not able to find a solution ;("<<endl;
            BSP_ERROR<<"I am sorry I quit"<<endl;
            exit(-1);
        }
        G.sort_V_Dec_move();/*sort for picking up variable nodes with the highest value of s_C*/
        G.choose_var_to_fix_and_clean();/*fix varaible nodes and clean the graph*/
        goto SP;/*go to SP*/

    } else {

        /*backtracking strategy*/

        /*The backtracking survey propagation (BSP) algorithms introduces a backtracking step,
         where a variable already assigned can be released and eventually re-assigned in a future decimation step.
         It is not difficult to understand when it is worth releasing a variable.
         The bias of a not assigned variable node i, which we recall to be defined as certitude:

                                                s_C=(1.-min(s_T,s_F),

         can be compared to the certitude of another variable node j already assigned:
         if the bias s_C associated to variable node j is smaller than the bias s_C associated to i
         then it is useful releasing variable j. Indeed, releasing variable j may help to correct
         mistakes made during previous decimation steps.
         In other words, decimation steps and backtracking steps choose variables,
         respectively, to  be fixed and to be relased, according to their biases s_C:
         variables to be fixed have the largest biases and variables to be released have the smallest biases.*/



        G.backtrack();/*bactrack move*/
        goto SP;/*go to SP*/
    }


PARAPHASE:

    /*When SP equations find trivial solutions, i.e. all messages from clauses to variables are null,
     the algorithm enter in a paramagnetic phase. The peculiarity of this phase is that the residual CNF formula
     obtaained by BSP is relatively simple for deterministic algorithms.
     We choose Walksat for the reason that is fast.
     In WalkSAT member in class Graph we also analyse if a solution is composed by frozen variable or not.
     This procedure will be described in detail in class Graph.*/

#ifdef WALKSAT
    G.print_on_file_residual_formula();/*print on file residual formula un-satisfied*/
    if(G.WalkSAT()) {
        G.print();
    };/*run walksat on residual formula*/
#endif

    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /*********************************** END BSP ***************************************/
    /***********************************************************************************/
    /***********************************************************************************/
    /***********************************************************************************/

    return 0;
}

