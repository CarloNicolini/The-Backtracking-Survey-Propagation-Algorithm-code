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




void help(const char *prog);/*print usage / help*/

int main(int argc,  char * const argv[]) {

    /* Strip logging flags so Graph still sees: prog -w K alpha N  or  prog -l file.cnf */
    vector<string> cleaned_args;
    cleaned_args.push_back(argv[0] ? argv[0] : "main");
    bool show_help = false;
    for (int i = 1; i < argc; ++i) {
        const string a = argv[i];
        if (a == "-h" || a == "--help") {
            show_help = true;
            continue;
        }
        if (a == "-q" || a == "--quiet") {
            bsp::set_log_level(bsp::LogLevel::Warn);
            continue;
        }
        if (a == "-v" || a == "--verbose") {
            bsp::set_log_level(bsp::LogLevel::Debug);
            continue;
        }
        if (a == "-vv") {
            bsp::set_log_level(bsp::LogLevel::Trace);
            continue;
        }
        if (a == "--log-prefix") {
            bsp::set_log_prefix(true);
            continue;
        }
        if (a.rfind("--log-level=", 0) == 0) {
            bsp::LogLevel level;
            if (!bsp::parse_log_level(a.substr(12), level)) {
                BSP_ERROR << "Unknown log level in " << a << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            bsp::set_log_level(level);
            continue;
        }
        if (a.rfind("--scorer=", 0) == 0) {
            if (!bsp_parse_scorer(a.substr(9))) {
                BSP_ERROR << "Unknown scorer in " << a
                          << " (expected cert|pol|i_c|gamma:<g>)" << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            continue;
        }
        if (a.rfind("--diag=", 0) == 0) {
            g_diag_prefix = a.substr(7);
            continue;
        }
        if (a == "--dump-residuals") {
            g_dump_residuals = true;
            continue;
        }
        if (a.rfind("--diag-every=", 0) == 0) {
            g_diag_every = static_cast<unsigned>(stoul(a.substr(13)));
            if (g_diag_every == 0) g_diag_every = 1;
            continue;
        }
        if (a.rfind("--r=", 0) == 0) {
            g_r_bsp = stod(a.substr(4));
            if (!(g_r_bsp >= 0.0 && g_r_bsp < 1.0)) {
                BSP_ERROR << "r must be in [0, 1), got " << a << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            continue;
        }
        if (a.rfind("--seed=", 0) == 0) {
            g_fixed_seed = stol(a.substr(7));
            if (g_fixed_seed < 1) {
                BSP_ERROR << "seed must be >= 1, got " << a << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            continue;
        }
        if (a.rfind("--theta=", 0) == 0) {
            g_bsp_theta = stod(a.substr(8));
            if (!(g_bsp_theta >= 0.0 && g_bsp_theta <= 1.0)) {
                BSP_ERROR << "theta must be in [0, 1], got " << a << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            continue;
        }
        if (a == "--veto") {
            g_veto = true;
            continue;
        }
        if (a.rfind("--eps=", 0) == 0) {
            g_epsilon = stod(a.substr(6));
            if (!(g_epsilon > 0.0)) {
                BSP_ERROR << "eps must be > 0, got " << a << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            continue;
        }
        if (a.rfind("--damping=", 0) == 0) {
            g_damping = stod(a.substr(10));
            if (!(g_damping >= 0.0 && g_damping < 1.0)) {
                BSP_ERROR << "damping must be in [0, 1), got " << a << endl;
                help(cleaned_args[0].c_str());
                return 1;
            }
            continue;
        }
        if (a == "--parisi-exchange") {
            g_parisi_exchange = true;
            continue;
        }
        if (a == "--parisi-audit") {
            g_parisi_audit = true;
            continue;
        }
        if (a == "--dynamic-i-backtrack") {
            g_dynamic_I_backtrack = true;
            continue;
        }
        if (a == "--two-stage-exchange") {
            g_two_stage_exchange = true;
            continue;
        }
        if (a.rfind("--dataset=", 0) == 0) {
            g_dataset_prefix = a.substr(10);
            continue;
        }
        if (a.rfind("--dataset-k=", 0) == 0) {
            g_dataset_k = static_cast<unsigned>(stoul(a.substr(12)));
            if (g_dataset_k == 0) g_dataset_k = 1;
            continue;
        }
        if (a.rfind("--dataset-every=", 0) == 0) {
            g_dataset_every = static_cast<unsigned>(stoul(a.substr(16)));
            if (g_dataset_every == 0) g_dataset_every = 1;
            continue;
        }
        if (a == "--oracle=off") {
            g_oracle = false;
            continue;
        }
        if (a == "--oracle" || a == "--oracle=on") {
            g_oracle = true;
            continue;
        }
        if (a.rfind("--minisat=", 0) == 0) {
            g_minisat_path = a.substr(10);
            continue;
        }
        if (a.rfind("--oracle-timeout=", 0) == 0) {
            g_oracle_timeout = static_cast<unsigned>(stoul(a.substr(17)));
            continue;
        }
        if (a == "--oracle-dir") {
            g_oracle_dir = true;
            continue;
        }
        if (a.rfind("--oracle-pick=", 0) == 0) {
            g_oracle_pick = static_cast<unsigned>(stoul(a.substr(14)));
            continue;
        }
        if (a.rfind("--lookahead-k=", 0) == 0) {
            g_lookahead_k = static_cast<unsigned>(stoul(a.substr(14)));
            continue;
        }
        if (a.rfind("--nn=", 0) == 0) {
            g_nn_path = a.substr(5);
            continue;
        }
        if (a.rfind("--nn-veto=", 0) == 0) {
            g_nn_veto = true;
            g_nn_cutoff = stod(a.substr(10));
            continue;
        }
        cleaned_args.push_back(a);
    }

    if (show_help || cleaned_args.size() < 2) {
        help(cleaned_args[0].c_str());
        return show_help ? 0 : 1;
    }
    if (!(cleaned_args[1] == "-w" || cleaned_args[1] == "-l")) {
        help(cleaned_args[0].c_str());
        return 1;
    }
    if (cleaned_args[1] == "-l" && cleaned_args.size() != 3) {
        help(cleaned_args[0].c_str());
        return 1;
    }
    if (cleaned_args[1] == "-w" && cleaned_args.size() < 5) {
        help(cleaned_args[0].c_str());
        return 1;
    }
    if (g_dump_residuals && g_diag_prefix.empty()) {
        BSP_ERROR << "--dump-residuals requires --diag=PREFIX" << endl;
        return 1;
    }
    if (g_oracle_pick > 0) {
        BSP_ERROR << "--oracle-pick is not implemented yet (use --oracle-dir)" << endl;
        return 1;
    }
    if (g_parisi_audit && !g_parisi_exchange) {
        BSP_ERROR << "--parisi-audit requires --parisi-exchange" << endl;
        return 1;
    }
    if (g_parisi_exchange && g_dynamic_I_backtrack) {
        BSP_ERROR << "--parisi-exchange and --dynamic-i-backtrack are exclusive" << endl;
        return 1;
    }
    if (g_two_stage_exchange &&
        (g_parisi_exchange || g_dynamic_I_backtrack || g_parisi_audit)) {
        BSP_ERROR << "--two-stage-exchange is exclusive with other Parisi schedulers" << endl;
        return 1;
    }
    if (!g_nn_path.empty()) {
        if (!bsp_nn_load(g_nn_path)) return 1;
    }

    vector<char *> av;
    av.reserve(cleaned_args.size());
    for (size_t i = 0; i < cleaned_args.size(); ++i)
        av.push_back(const_cast<char *>(cleaned_args[i].c_str()));
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
    int c;
    bool have_instance = false;
    while ((c = getopt (ac, &av[0], "l:w:h")) != -1)/* write or load an instance K-SAT*/
        switch (c) {
        case 'l':/* load an instance K-SAT*/
            G.read_from_file_graph(); /*read CNF instance from file*/
            have_instance = true;
            break;
        case 'w':/* write an instance for SAT problem*/
            G.write_on_file_graph(); /*build and write a CNF instance on file*/
            have_instance = true;
            break;

        case 'h':/* help function is called*/
            help(cleaned_args[0].c_str());
            return 0;

        default:
            help(cleaned_args[0].c_str());
            return 1;
        }
    if (!have_instance) {
        help(cleaned_args[0].c_str());
        return 1;
    }
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
    if(g_two_stage_exchange)BSP_INFO<<"START TWO-STAGE PARISI EXCHANGE BSP:"<<endl;
    else if(g_parisi_exchange)BSP_INFO<<"START PARAMETER-FREE PARISI EXCHANGE BSP:"<<endl;
    else if(g_r_bsp!=0.)BSP_INFO<<"START BSP WITH r="<<g_r_bsp<<":"<<endl;
    else BSP_INFO<<"START SID:"<<endl;
    BSP_INFO<<"Decimation scorer: "<<bsp_scorer_name()<<endl;
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
    if(g_parisi_exchange && G.complexity!=0.)
        G.prepare_parisi_step();/*state-dependent eq. (5) move*/
    if(g_two_stage_exchange && G.complexity!=0.)
        G.prepare_two_stage_step();/*release, re-equilibrate, replace*/
    if(g_parisi_audit)G.audit_parisi_release();/*expensive estimator validation*/
    G.diag_step();/*log SP fixed point (no-op unless --diag)*/
    G.dataset_trials();/*tentative-fix trials (no-op unless --dataset)*/
    G.apply_nn_scores();/*overwrite scores if --nn=weights was given*/

    if(g_two_stage_exchange) {
        BSP_INFO<<G;
        G.save();
        if(G.complexity==0)goto PARAPHASE;
        if(G.complexity<-2. && !G.two_stage_will_release()) {
            BSP_ERROR<<"Negative complexity and no two-stage release"<<endl;
            BSP_ERROR<<"I am sorry I quit"<<endl;
            exit(-1);
        }
        G.apply_two_stage_step();
        goto SP;
    }

    if(g_parisi_exchange) {
        BSP_INFO<<G;
        G.save();
        if(G.complexity==0)goto PARAPHASE;
        if(G.complexity<-2. && !G.parisi_will_exchange()) {
            BSP_ERROR<<"Negative complexity and no improving Parisi exchange"<<endl;
            BSP_ERROR<<"I am sorry I quit"<<endl;
            exit(-1);
        }
        G.apply_parisi_step();
        goto SP;
    }

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


void help(const char *prog) {
    const char *name = (prog != NULL && prog[0] != '\0') ? prog : "main";
    cout << "Usage:\n"
         << "  " << name << " [log options] -w <K> <alpha> <N>\n"
         << "  " << name << " [log options] -l <formula.cnf>\n"
         << "  " << name << " -h\n"
         << "\n"
         << "Solver options:\n"
         << "  -w <K> <alpha> <N>   Generate a random K-SAT instance and solve it.\n"
         << "                       K     = literals per clause\n"
         << "                       alpha = clause density (M/N)\n"
         << "                       N     = number of variables\n"
         << "  -l <formula.cnf>    Load a CNF formula from file and solve it.\n"
         << "  -h, --help          Show this help message.\n"
         << "  --scorer=SPEC       Decimation scorer: cert|pol|i_c|gamma:<g>.\n"
         << "                       Default keeps the compiled-in scorer (CERT).\n"
         << "  --diag=PREFIX       Write PREFIX_steps.csv / PREFIX_vars.csv\n"
         << "                       per-SP-fixed-point diagnostics (off by default).\n"
         << "  --diag-every=K      Log vars + residuals every K steps (default 1).\n"
         << "  --dump-residuals    Also dump PREFIX_res_s<step>.cnf (needs --diag).\n"
         << "  --r=R             Backtracking ratio in [0, 1) (default 0.9;\n"
         << "                       0 gives SID without backtracking).\n"
         << "  --seed=S          Fix the RNG seed S >= 1 for reproducible runs.\n"
         << "  --theta=T         Fix only vars with direction margin\n"
         << "                       |sT-sF|/(sT+sF) >= T in [0, 1] (default 0).\n"
         << "  --veto            Veto co-decimating vars sharing a clause.\n"
         << "  --eps=E           SP convergence threshold (default 0.01).\n"
         << "  --damping=D       SP update damping in [0, 1) (default 0).\n"
         << "  --parisi-exchange Choose decimation or an iso-size fix/release\n"
         << "                       exchange from Parisi's P_max > I_min rule.\n"
         << "  --parisi-audit    Fork one exact release trial per exchange\n"
         << "                       opportunity for I_min validation.\n"
         << "  --dynamic-i-backtrack Rank fixed variables by current Parisi I(k)\n"
         << "                       during the standard BSP backtracking schedule.\n"
         << "  --two-stage-exchange Release I_min, reconverge, choose a replacement,\n"
         << "                       then force one net-progress decimation.\n"
         << "  --dataset=PREFIX  Write PREFIX_dataset.csv with tentative-fix\n"
         << "                       DeltaSigma trials (off by default, POSIX).\n"
         << "  --dataset-k=K     Shortlist size per step (default 50).\n"
         << "  --dataset-every=S Trial cadence in SP steps (default 1).\n"
         << "  --oracle / --oracle=on  Label each trial residual with minisat.\n"
         << "  --oracle=off       Skip the exact minisat label (default).\n"
         << "  --minisat=PATH     Minisat binary (default minisat).\n"
         << "  --oracle-timeout=S Per-trial minisat seconds (default 10, 0=off).\n"
         << "  --oracle-dir      Resolve each decimation direction by minisat.\n"
         << "  --oracle-pick=K   Scan top-K for a SAT-preserving move (0=off).\n"
         << "  --lookahead-k=K  Try both directions for the top-K scored vars;\n"
         << "                       fix the converged move with maximum Sigma (0=off).\n"
         << "  --nn=FILE         Rank by a GenANN weights file from bsp-train.\n"
         << "  --nn-veto=C       Veto mode: bury vars scoring below C,\n"
         << "                       keep hand-crafted bias otherwise (needs --nn).\n"
         << "\n"
         << "Logging options (do not change the algorithm):\n"
         << "  -q, --quiet         Warnings and errors only\n"
         << "  -v, --verbose       Debug logging\n"
         << "  -vv                 Trace logging\n"
         << "  --log-level=LEVEL   error|warn|info|debug|trace (default: info)\n"
         << "  --log-prefix        Prefix each line with [LEVEL]\n"
         << "\n"
         << "Examples:\n"
         << "  " << name << " -w 3 4.0 50\n"
         << "  " << name << " -v --log-prefix -w 3 4.0 50\n"
         << "  " << name << " -l formula.cnf\n";
}
