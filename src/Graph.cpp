//
//  Graph.cpp
//  TheBacktrackingSurveyPropagation
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
//  Created by Raffaele Marino on 26/11/2018.
//  Copyright © 2018 Raffaele Marino. All rights reserved.
//

#include <bsp/Graph.hpp>
#include <bsp/thermo_sp.hpp>
#include <bsp/softq.hpp>
#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <csignal>
#include <cerrno>
#endif

/*Phase 1 diagnostic options (see Header.h).*/
string g_diag_prefix;
unsigned g_diag_every = 1;
bool g_dump_residuals = false;
double g_r_bsp = _R_BSP;
long g_fixed_seed = -1;
double g_bsp_theta = 0.0;
bool g_veto = false;
double g_epsilon = epsilon;
double g_damping = 0.0;
string g_minisat_path = "minisat";
unsigned g_oracle_timeout = 10;
bool g_oracle_dir = false;
unsigned g_oracle_pick = 0;
unsigned g_lookahead_k = 0;
bool g_dynamic_i = false;
bool g_fe_backtrack = false;
double g_bt_cost = 0.4;
bool g_corr_batch = false;
bool g_adaptive_r = false;
double g_frac = frac;
/*A step is steep when it removes this fraction of the current complexity.
 SP is slow when it needs more than this many iterations (t_max is 1024).
 The boost is added to r, then clipped below 1.*/
static const double k_slope_frac = 0.05;
static const unsigned k_eta_cut = 128;
static const double k_r_boost = 0.09;
/*Score of a failed probe. -inf is unsafe under -ffast-math.*/
static const double k_softq_fail = -1e300;
/*Wall-clock limit of one rollout child.*/
static const unsigned k_rollout_timeout_s = 1800;

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***************************** START CLAUSE CLASS **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


/*public member class Clause. This member helps to clean the graph from satisfied clauses and
 from unsitisfied literal in unsitisified clauses. It starts to store the value of the message from the clause j
 to the variable i, the one which has been fixed previously. The member store the survey in survey_cl_to_i_f
 and set in variable i the survey _surveys_cl_to_i[j] equl to 1. Then check if the clause is satisfied or not*/

void Clause::clean(Vertex * &p_V, unsigned int &j) { /*clean the clasue*/
    bool flag_deg=true;
    survey_cl_to_i_f[*imag(p_V->_I_am_in_cl_at_init[j])]=p_V->_surveys_cl_to_i[j];/*store clause to variable survey in the clause*/
    if(cp_lit[*imag(p_V->_I_am_in_cl_at_init[j])]) {
        (p_V->_who_I_am)?p_V->_surveys_cl_to_i[j]=0.:p_V->_surveys_cl_to_i[j]=1.;/*set survey clause to variable  into a Vertex object to one*/
        _vb[*imag(p_V->_I_am_in_cl_at_init[j])]=p_V->_who_I_am;/*update bool vector  _vb*/
    } else {
        (!p_V->_who_I_am)?p_V->_surveys_cl_to_i[j]=0.:p_V->_surveys_cl_to_i[j]=1.;/*set survey clause to variable  into a Vertex object to one*/
        _vb[*imag(p_V->_I_am_in_cl_at_init[j])]=(!(p_V->_who_I_am));/*update bool vector  _vb*/
    }
    _v_ref[*imag(p_V->_I_am_in_cl_at_init[j])]=_cast_b_to_i(_vb[*imag(p_V->_I_am_in_cl_at_init[j])]);
    _go_forward[*imag(p_V->_I_am_in_cl_at_init[j])]=0;
    _lit.erase(p_V->_lit_list_i[j]);/*erase literal from the clause*/
    V.erase(p_V->where_I_am[j]);/*erase variable form the clause*/
    _it_list_V_begin=V.begin();
    _size_cl--;
    survey_cl_to_i.erase(p_V->_where_surveys_are_in_cl[j]);/*erase the survey clause to i from the clause*/
    survey_begin=survey_cl_to_i.begin();
    if(I_am_a_cl_true) {
        flag_deg=false;
    }
    I_am_a_cl_true=_logic_operator();/*check if the clause has been satisfied or not*/
    if(flag_deg and I_am_a_cl_true) {
        /*if the clause has been satisfied then save surveys, set the other variables surveys to 0  and */
        for (unsigned int i=0; i<survey_cl_to_i_f.size(); ++i) {
            if(survey_cl_to_i_f[i]==-1.) {
                survey_cl_to_i_f[i]=*survey_cl_to_i_f_ptr[i];
                *survey_cl_to_i_f_ptr[i]=0.;
            }
        }
        /*update the variable nodes degree of the other variables that have not been fixed yet.*/
        for (list<Vertex *>::iterator __it=V.begin(); __it!=V.end(); ++__it) {
            (*__it)->_degree_i--;
            if((*__it)->_degree_i<0) {
                BSP_ERROR<<"Error degree node j less than 0"<<endl;
                BSP_ERROR<<"Il grado e':"<<(*__it)->_degree_i<<" "<<(*__it)->_vertex<<endl;
                exit(-1);
            }
        }

    }
}


/*public member class Clause. This member builds the clauses unsitisfied by the new assignment into the graph.*/
void Clause::build(Vertex * &p_V, unsigned int &j) { /*build the clasue*/
    /*update the value of the survey in the clause to the last one*/
    /* and rebuild the clause*/
    this->_vb[*imag(p_V->_I_am_in_cl_at_init[j])]=false;/*set to default value*/
    this->I_am_a_cl_true=_logic_operator();/*check if the clause has been satisfied or not by another assignment*/
    this->_v_ref[*imag(p_V->_I_am_in_cl_at_init[j])]=-1;/*set to default value*/
    this->V.push_front(v_V[*imag(p_V->_I_am_in_cl_at_init[j])]);/*build variable into the clause*/
    this->_lit.push_front(this->cp_lit[*imag(p_V->_I_am_in_cl_at_init[j])]);/*build literal into the clause*/
    p_V->where_I_am[j]=(this->V.begin());/*store iterator value*/
    p_V->_lit_list_i[j]=(this->_lit.begin());/*store iterator value*/
    this->_it_list_V_begin=V.begin();/*store iterator value*/
    this->_size_cl++;/*update clause size value*/
    this->_go_forward[*imag(p_V->_I_am_in_cl_at_init[j])]=1;/*set to default value*/
    this->survey_cl_to_i.push_front(this->v_survey_cl_to_i[*imag(p_V->_I_am_in_cl_at_init[j])]);/*build the survey clause to i from the clause*/
    p_V->_where_surveys_are_in_cl[j]=(this->survey_cl_to_i.begin());/*update iterator value*/
    this->survey_begin=survey_cl_to_i.begin();/*update iterator value*/
    if(!(this->I_am_a_cl_true)) {
        p_V->_surveys_cl_to_i[j]=this->survey_cl_to_i_f[*imag(p_V->_I_am_in_cl_at_init[j])];
        /*update the variable nodes degree of the other variables that have not been fixed yet.*/
        if(!(this->_I_am_in_list_unsat)) {
            for (unsigned int i=0; i<this->survey_cl_to_i_f.size(); ++i) {/*restore all surveys of the clause*/
                if(this->survey_cl_to_i_f[i]!=-1. and this->_go_forward[i]==1) {
                    *(this->survey_cl_to_i_f_ptr[i])=this->survey_cl_to_i_f[i];
                    *(this->v_survey_cl_to_i[i])=this->survey_cl_to_i_f[i];
                    this->survey_cl_to_i_f[i]=-1;
                }
            }
            for (list<Vertex *>::iterator __it=this->V.begin(); __it!=this->V.end(); ++__it) {
                (*__it)->_degree_i++;/*update the variable nodes degree of the other variables that have not been fixed yet.*/
            }
        } else {
            /*The litteral in an unsat clause is re-build*/
            *(this->survey_cl_to_i_f_ptr[*imag(p_V->_I_am_in_cl_at_init[j])])=this->survey_cl_to_i_f[*imag(p_V->_I_am_in_cl_at_init[j])];
            *(this->v_survey_cl_to_i[*imag(p_V->_I_am_in_cl_at_init[j])])=this->survey_cl_to_i_f[*imag(p_V->_I_am_in_cl_at_init[j])];
            this->survey_cl_to_i_f[*imag(p_V->_I_am_in_cl_at_init[j])]=-1.;/*update clause to variable survey*/
            p_V->_degree_i++;/*update the variable node degree */
        }

    } else {
        /*set the survey in the ture clause at 0., it will be set to the last value when the clause will be unsat*/
        p_V->_surveys_cl_to_i[j]=0.;
    }

}

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/******************************  END CLAUSE CLASS **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/



/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/*****************************  START GRAPH CLASS **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/********************************** START SP ***************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


/*public member class Graph which describe unit propagation algorithm. This member is called when a clause is
 composed only by one literal, and therefore has to be satisfied, otherwise we obtain a contraddiction*/
void Graph::unit_propagation(unsigned int  &c) { /*fix the variable into clause c to true and clean the graph*/
    //cout<<"unit propagation"<<endl;
    _unit_prop++;/*update counter unit propagation*/
    if(_cl[c].size()>1 and _new!=1)exit(-1);/*check that we have the right clause*/
    if(_cl[c].empty())exit(-1);/*check that we have the right clause*/
    if (*_cl[c]._lit.begin()) {
        (*_cl[c].V.begin())->_who_I_am=true; /*set the variable to the value that satisfied the literal into the clause*/
        (*_cl[c].V.begin())->_sT=1.;/*set the associated survey to one*/
        (*_cl[c].V.begin())->_sF=0.;
        (*_cl[c].V.begin())->_sI=0.;
    } else {
        (*_cl[c].V.begin())->_who_I_am=false;/*set the variable to the value that satisfied the literal into the clause*/
        (*_cl[c].V.begin())->_sT=0.;
        (*_cl[c].V.begin())->_sF=1.;/*set the associated survey to one*/
        (*_cl[c].V.begin())->_sI=0.;
    }
    (*_cl[c].V.begin())->_sC=1.1;/*set the certitude to 1.1 to be sure that in the sort the right variable shows up*/
    (*_cl[c].V.begin())->_degree_i=0;/*set variable node degree to 0*/
    Vertex * cp=(*_cl[c].V.begin()); /*variable node copy*/
    sort(ptrV.begin()+(_list_fixed_element.size()), ptrV.end(), _Vertex_greater_pred());/*sort the vector*/
    (*_cl[c].V.begin())->_sC=1.;/*set the certitude to 1*/
    _list_fixed_element.push_back(cp);/*store the Vertex in the listt of fixed element*/
    cp->_it_list_fixed_elem=--_list_fixed_element.end();/*save its address into the varibale node*/
    cp->_I_am_a_fixed_variable=true;/*fix the variable*/
    cp->_forced_by_up=true;
    diag_move("up", cp, cp->_who_I_am ? 1 : 0);
    /*clean the graph*/
    /*
     Clean the graph means:

     1) erasing from the factor graph all satisfied clauses;
     2) erasing literals associated to variable i, which are present in a clause that is not satisfied by the variable node assignement.
     */
    clean(cp);
    _N_t=_N-static_cast<unsigned int>(_list_fixed_element.size());/*update the number of un-fixed variable nodes*/
}

/*public member class Graph. Fixes one variable during decimation: stores it in
 the fixed list, sets its value, logs the move and cleans the graph.
 force_dir 0/1 overrides the SP rule; with --oracle-dir the direction is
 resolved by exact SAT checks (SP-preferred first, flipped if fatal).*/
void Graph::decimate_one(Vertex* v, int force_dir) {
    int dir=force_dir;
    if (dir < 0 && g_oracle_dir && !_oracle_disabled) dir=oracle_dir(v);
    _list_fixed_element.push_back(v);/*store the variable node into fixed element list*/
    v->_it_list_fixed_elem=--_list_fixed_element.end();/*save its address into the varibale node*/
    v->_I_am_a_fixed_variable=true;/*fix the variable*/
    if (dir < 0) v->fix_var_i();/*SP rule*/
    else v->_who_I_am=(dir==1);/*forced direction*/
    diag_move("dec", v, v->_who_I_am ? 1 : 0);
    if(_last_certitude>v->_sC)_last_certitude=v->_sC;/*store last certitude*/

    /*clean the graph*/
    /*
     Clean the graph means:

     1) erasing from the factor graph all satisfied clauses;
     2) erasing literals associated to variable i, which are present in clauses not satisfied by the variable node assignement.
     */
    clean(v);
    v->_degree_i=0;/*set to 0 the degree of the variable node*/
}

/*public member class Graph. True if v shares an unsatisfied clause with any
 variable already selected in sel (factor-graph distance 2 veto, Phase 2).
 NOTE: selected vars were just fixed and erased from V, so membership is
 tested on cpV (original members); satisfied clauses are skipped.*/
bool Graph::vetoed(Vertex* v, const vector<Vertex*>& sel) {
    for (unsigned j=0; j<v->_I_am_in_cl_at_init.size(); ++j) {
        unsigned c=*real(v->_I_am_in_cl_at_init[j]);
        if (!_cl[c]._I_am_in_list_unsat) continue;
        for (unsigned m=0; m<_cl[c].cpV.size(); ++m) {
            for (unsigned k=0; k<sel.size(); ++k)
                if (_cl[c].cpV[m]==sel[k]) {
                    BSP_DEBUG<<"veto: v"<<v->_vertex<<" shares clause "<<c<<endl;
                    return true;
                }
        }
    }
    return false;
}

static bool same_var_set(const vector<Vertex*>& a, const vector<Vertex*>& b) {
    if (a.size()!=b.size()) return false;
    for (unsigned i=0; i<a.size(); ++i) {
        bool found=false;
        for (unsigned j=0; j<b.size(); ++j)
            if (a[i]==b[j]) { found=true; break; }
        if (!found) return false;
    }
    return true;
}

/*Place win at the front of the unfixed region. The fixed prefix stays put.*/
static void promote_unfixed(vector<Vertex*>& p, unsigned nfixed, const vector<Vertex*>& win) {
    vector<Vertex*> rest;
    rest.reserve(p.size()-nfixed);
    for (unsigned i=nfixed; i<p.size(); ++i) {
        bool used=false;
        for (unsigned j=0; j<win.size(); ++j)
            if (p[i]==win[j]) { used=true; break; }
        if (!used) rest.push_back(p[i]);
    }
    for (unsigned j=0; j<win.size(); ++j) p[nfixed+j]=win[j];
    for (unsigned j=0; j<rest.size(); ++j) p[nfixed+win.size()+j]=rest[j];
}

/*Sum of the soft site values v_alpha over the free variables p[from..],
 one sum for each (q, alpha) pair.*/
static vector<double> free_site_value_sums(const vector<Vertex*>& p, unsigned from,
                                           const vector<pair<double, double> >& qa) {
    vector<double> s(qa.size(), 0.);
    for (unsigned i=from; i<p.size(); ++i)
        for (unsigned g=0; g<qa.size(); ++g)
            s[g]+=softq_site_value(p[i]->_sT, p[i]->_sF, qa[g].first, qa[g].second);
    return s;
}

/*Fork a child that fixes vars in the SP direction and reconverges.
 The parent graph is unchanged. Returns 1 if that child found a fixed point.
 sigma is the complexity after reconvergence. sweeps counts the iterations
 spent, and is added to the run total so a lookahead can be compared with
 plain BSP at equal SP work. A contradiction or a non-convergence returns 0
 and charges t_max sweeps. If vsum is given, it receives, for each (q, alpha)
 pair of qa, the sum of the soft site values over the free variables after
 reconvergence.*/
int Graph::probe_fixes(const vector<Vertex*>& vars, double& sigma, int& sweeps,
                       const vector<pair<double, double> >& qa, vector<double>* vsum) {
    sigma=0.;
    sweeps=0;
    if (vsum) vsum->assign(qa.size(), 0.);
    if (vars.empty()) return 0;
#ifdef _WIN32
    return 0;
#else
    const size_t nout=3+qa.size();
    double* tout=(double*)mmap(NULL, nout*sizeof(double), PROT_READ|PROT_WRITE,
                               MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (tout==MAP_FAILED) {
        BSP_ERROR<<"mmap failed for complexity probe"<<endl;
        exit(-1);
    }
    fill(tout, tout+nout, 0.);
    _diag_step_out<<flush;
    _diag_var_out<<flush;
    _diag_move_out<<flush;
    cout<<flush;
    cerr<<flush;
    fflush(NULL);
    pid_t pid=fork();
    if (pid<0) {
        BSP_ERROR<<"fork failed for complexity probe"<<endl;
        exit(-1);
    }
    if (pid==0) {
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        _in_trial=true;
        for (unsigned i=0; i<vars.size(); ++i) {
            Vertex* v=vars[i];
            _list_fixed_element.push_back(v);
            v->_it_list_fixed_elem=--_list_fixed_element.end();
            v->_I_am_a_fixed_variable=true;
            v->fix_var_i();
            if (_last_certitude>v->_sC) _last_certitude=v->_sC;
            clean(v);
            v->_degree_i=0;
        }
        stable_partition(ptrV.begin(), ptrV.end(), _Vertex_is_fixed_pred());
        convergence_messages();
        surveys();
        tout[0]=1.;
        tout[1]=complexity;
        tout[2]=(double)_time_conv_print;
        vector<double> s=free_site_value_sums(ptrV, (unsigned)_list_fixed_element.size(), qa);
        copy(s.begin(), s.end(), tout+3);
        _exit(0);
    }
    int status=0;
    while (waitpid(pid, &status, 0)<0 && errno==EINTR) { /*retry*/ }
    int conv=(WIFEXITED(status) && WEXITSTATUS(status)==0) ? 1 : 0;
    if (conv) {
        sigma=tout[1];
        sweeps=(int)tout[2]+1;
        if (vsum) copy(tout+3, tout+nout, vsum->begin());
    } else {
        sweeps=t_max;
    }
    _sp_sweeps+=(unsigned long)sweeps;
    munmap(tout, nout*sizeof(double));
    return conv;
#endif
}
/*Soft two-step look-ahead (paper, Section softq). The candidates are the
 top-M free variables in the BSP order. Each is probed: fixed in the SP
 direction, SP reconverged in a forked child. Its score is
 alpha log pi(s|k) + D(k,s), with D = V(after) - V(before) + v_k.
 A probe that hits a contradiction or does not converge scores -inf.
 The best batch moves to the front of the free region; alpha>0 samples it
 with the Gumbel top-k trick, i.e. without replacement from e^{score/alpha}.
 At a rollout checkpoint every candidate also runs plain BSP to the end in a
 child of its own. Returns true only in such a child, which must fix exactly
 the candidate now at the front.*/
bool Graph::softq_step(unsigned nfixed, unsigned batch) {
    vector<Vertex*> cand;
    for (unsigned i=nfixed; i<_N && cand.size()<g_softq_m; ++i)
        if (bsp_pass_margin(ptrV[i]->_sT, ptrV[i]->_sF)) cand.push_back(ptrV[i]);
    bool rollout=_softq_next_rollout<g_softq_rollout_at.size() &&
                 nfixed>=g_softq_rollout_at[_softq_next_rollout]*_N;
    if (cand.size()<=batch && !rollout) return false;
    /*qa[0] is the active (q, alpha); a rollout also logs the scores of the grid*/
    vector<pair<double, double> > qa(1, make_pair(g_softq_q, g_softq_alpha));
    if (rollout) qa.insert(qa.end(), g_softq_score_grid.begin(), g_softq_score_grid.end());
    vector<double> v0=free_site_value_sums(ptrV, nfixed, qa), v1;
    vector<vector<double> > score(qa.size(), vector<double>(cand.size(), k_softq_fail));
    vector<double> sig(cand.size(), 0.);
    unsigned nfail=0, swsum=0;
    for (unsigned j=0; j<cand.size(); ++j) {
        Vertex* v=cand[j];
        int sw=0;
        bool ok=probe_fixes(vector<Vertex*>(1, v), sig[j], sw, qa, &v1);
        swsum+=sw;
        if (!ok) { ++nfail; continue; }
        for (unsigned g=0; g<qa.size(); ++g) {
            double q=qa[g].first, a=qa[g].second;
            score[g][j]=softq_log_policy(v->_sT, v->_sF, v->_sT>v->_sF, q, a)
                        +v1[g]-v0[g]+softq_site_value(v->_sT, v->_sF, q, a);
        }
    }
    BSP_DEBUG<<"softq probes="<<cand.size()<<" failed="<<nfail<<" sweeps="<<swsum<<endl;
    if (rollout) {
        ofstream out("softq_rollouts.csv", ios::app);
        for (unsigned j=0; j<cand.size(); ++j) {
            out<<setprecision(12)<<g_softq_rollout_at[_softq_next_rollout]<<","<<nfixed<<","<<j<<","
               <<cand[j]->_vertex<<","<<cand[j]->_sC<<","<<sig[j]<<","<<complexity;
            for (unsigned g=0; g<qa.size(); ++g) out<<","<<score[g][j];
            out<<"\n";
        }
        out.close();
        if (softq_rollout(cand, _softq_next_rollout)) return true;
        ++_softq_next_rollout;
    }
    if (g_softq_observe || cand.size()<=batch) return false;
    vector<double> key(score[0]);
    if (g_softq_alpha>0.) {
        for (unsigned j=0; j<key.size(); ++j) {
            double u=(random()+0.5)/(RAND_MAX+1.0);
            key[j]+=-g_softq_alpha*log(-log(u));
        }
    }
    vector<unsigned> order(cand.size());
    for (unsigned j=0; j<order.size(); ++j) order[j]=j;
    stable_sort(order.begin(), order.end(), [&key](unsigned a, unsigned b) { return key[a]>key[b]; });
    vector<Vertex*> win;
    for (unsigned j=0; j<batch; ++j) win.push_back(cand[order[j]]);
    promote_unfixed(ptrV, nfixed, win);
    BSP_DEBUG<<"softq chose rank "<<order[0]<<" of "<<cand.size()<<" sp_sweeps="<<_sp_sweeps<<endl;
    return false;
}

/*Forks one child per candidate at rollout checkpoint idx. Each child writes
 into rollout_f<f>_c<j>/, fixes its candidate alone and continues with plain
 BSP. The parent waits for all children and returns false. A child returns
 true with its candidate at the front of the free region.*/
bool Graph::softq_rollout(const vector<Vertex*>& cand, unsigned idx) {
#ifdef _WIN32
    return false;
#else
    _diag_step_out<<flush;
    _diag_var_out<<flush;
    _diag_move_out<<flush;
    cout<<flush;
    cerr<<flush;
    fflush(NULL);
    unsigned nfixed=(unsigned)_list_fixed_element.size();
    vector<pid_t> kids;
    for (unsigned j=0; j<cand.size(); ++j) {
        pid_t pid=fork();
        if (pid<0) {
            BSP_ERROR<<"fork failed for softq rollout"<<endl;
            exit(-1);
        }
        if (pid>0) {
            kids.push_back(pid);
            continue;
        }
        ostringstream dir;
        dir<<"rollout_f"<<g_softq_rollout_at[idx]<<"_c"<<j;
        mkdir(dir.str().c_str(), 0755);
        if (chdir(dir.str().c_str())!=0) _exit(1);
        if (!freopen("log.txt", "w", stdout) || !freopen("log.txt", "a", stderr)) _exit(1);
        bsp::set_log_level(bsp::LogLevel::Info);
        alarm(k_rollout_timeout_s);
        _diag_step_out.close();
        _diag_var_out.close();
        _diag_move_out.close();
        _diag_header_done=false;
        g_softq_m=0;
        g_softq_rollout_at.clear();
        promote_unfixed(ptrV, nfixed, vector<Vertex*>(1, cand[j]));
        return true;
    }
    for (unsigned j=0; j<kids.size(); ++j) {
        int status=0;
        while (waitpid(kids[j], &status, 0)<0 && errno==EINTR) { /*retry*/ }
    }
    return false;
#endif
}

/*public member class Graph which helps us to fix the variables that have the highest value of certitude*/
void Graph::choose_var_to_fix_and_clean() {
    /*set the rangee over variables unfixed are into vertex ptrV*/
    _M_t=0;
    unsigned int _size=(unsigned int)(g_frac*((double)_N_t))+(unsigned int)_list_fixed_element.size();
    unsigned int _size_init=(unsigned int)_list_fixed_element.size();
    /*set unit propagation counter to zero*/
    _unit_prop=0;
    /*condition for erasing only one variable at each convergence when (frac*((double)_N_t)<1*/
    if (_size<=_size_init) {
        _size=1+_size_init;
    }
    unsigned int _batch=_size-_size_init;/*decimation width of this move*/
    /*Variant 2 needs a real batch. Below that width it is the same as the
     2016 ranking, so the single-variable lookahead (variant 1) runs instead.
     Both leave the SP direction alone and only reorder which variables are fixed.*/
    if (g_softq_m>0) {
        if (softq_step(_size_init, _batch)) _batch=1;/*rollout child: its candidate alone*/
    } else if (g_corr_batch && _batch>=2) {
        vector<Vertex*> legacy, diverse;
        for (unsigned i=_size_init; i<_N && legacy.size()<_batch; ++i) {
            if (!bsp_pass_margin(ptrV[i]->_sT, ptrV[i]->_sF)) continue;
            legacy.push_back(ptrV[i]);
        }
        for (unsigned i=_size_init; i<_N && diverse.size()<_batch; ++i) {
            if (!bsp_pass_margin(ptrV[i]->_sT, ptrV[i]->_sF)) continue;
            if (vetoed(ptrV[i], diverse)) continue;
            diverse.push_back(ptrV[i]);
        }
        if (legacy.size()==_batch && diverse.size()==_batch && !same_var_set(legacy, diverse)) {
            double s_leg=0., s_div=0.;
            int w_leg=0, w_div=0;
            int c_leg=probe_fixes(legacy, s_leg, w_leg);
            int c_div=probe_fixes(diverse, s_div, w_div);
            bool take_div=c_div && std::isfinite(s_div) &&
                          (!c_leg || !std::isfinite(s_leg) || s_div>s_leg);
            if (take_div) promote_unfixed(ptrV, _size_init, diverse);
            BSP_INFO<<"corr-batch "<<(take_div?"diverse":"legacy")
                    <<" Sigma_leg="<<s_leg<<"("<<c_leg<<")"
                    <<" Sigma_div="<<s_div<<"("<<c_div<<")"
                    <<" sp_sweeps="<<_sp_sweeps<<endl;
        }
    } else if (g_lookahead_k>1) {
        unsigned nfree=_N-_size_init;
        unsigned k=g_lookahead_k<nfree ? g_lookahead_k : nfree;
        int best=-1;
        double best_sigma=0.;
        unsigned probed=0;
        for (unsigned j=0; j<k; ++j) {
            Vertex* v=ptrV[_size_init+j];
            if (!bsp_pass_margin(v->_sT, v->_sF)) continue;
            vector<Vertex*> one(1, v);
            double sigma=0.;
            int sw=0;
            int conv=probe_fixes(one, sigma, sw);
            ++probed;
            if (!conv || !std::isfinite(sigma)) continue;
            if (best<0 || sigma>best_sigma) {
                best=(int)j;
                best_sigma=sigma;
            }
        }
        if (best>0) {
            vector<Vertex*> win(1, ptrV[_size_init+best]);
            promote_unfixed(ptrV, _size_init, win);
        }
        if (probed>0)
            BSP_INFO<<"lookahead chose rank "<<(best<0?0:best)
                    <<" of "<<probed<<" Sigma="<<best_sigma
                    <<" sp_sweeps="<<_sp_sweeps<<endl;
    }
    vector<Vertex*> _veto_sel;
    _veto_sel.reserve(_batch);
    unsigned int _counter_dec_var=0;
    for (unsigned int i=_size_init; i<_N && _counter_dec_var<_batch; ++i) {
        if (!bsp_pass_margin(ptrV[i]->_sT, ptrV[i]->_sF)) continue;/*theta skip*/
        if (g_veto && vetoed(ptrV[i], _veto_sel)) continue;/*distance-2 veto*/
        decimate_one(ptrV[i]);
        _veto_sel.push_back(ptrV[i]);
        _counter_dec_var++;
    }
    if (_counter_dec_var==0) {
        /*nothing passed the gates: legacy top-1 fallback to guarantee progress*/
        BSP_WARN<<"theta/veto skipped all unfixed vars: legacy top-1 fallback"<<endl;
        decimate_one(ptrV[_size_init]);
        _counter_dec_var=1;
    }
    /*skips may scatter fixes: restore the fixed-first ptrV partition*/
    stable_partition(ptrV.begin(), ptrV.end(), _Vertex_is_fixed_pred());

    _m_t_m_1=_counter_dec_var;
    _N_t=_N-static_cast<unsigned int>(_list_fixed_element.size());/*update the number of un-fixed variable nodes*/
    //update_products();
}

void Graph::clean(Vertex * &V_to_clean) {
    unsigned int k=0;
    unsigned int temp;
    for (unsigned int j=0; j<V_to_clean->_surveys_cl_to_i.size(); ++j) {
        k=*real(V_to_clean->_I_am_in_cl_at_init[j]);
        _cl[k].clean(V_to_clean, j);/*clean the graph*/
        if(_cl[k].I_am_a_cl_true and _cl[k]._I_am_in_list_unsat) {
            /*At this point the algorithm  picks a clause sat and set it on the top of the vector vec_list*/
            /*The vector vec_list_cl contains pointers to clauses sat and unsat. The value _m divides the sat clause, from 0 to _m-1, to unsat clause,
             from _m to  _M*/

            temp=_cl[k]._l;
            swap(vec_list_cl[_cl[k]._l], vec_list_cl[_m]);
            (vec_list_cl[_cl[k]._l])->_l=temp;
            (vec_list_cl[_m])->_l=_m;
            _cl[k]._I_am_in_list_unsat=false;
            _cl_list.erase(_cl[k]._it_list);
            _m++;
        }
    }
}

/*public member class Graph. This memeber sorts in descending order, using as predicate the certitude, vertex
 vector ptrV. The first x components of the vector are not sorted because they contain fixed variables*/
void Graph::sort_V_Dec_move() {
    sort(ptrV.begin()+(_list_fixed_element.size()), ptrV.end(), _Vertex_greater_pred());
}



/*public memeber class Graph. This memeber computes the surveys for each variable node.
 It is called when a convergence of all messages from a clause to variable is found.
 Moreover this member computes the variable complexity associated to each variable node*/
void Graph::surveys() { /*compute surveys for each variable node*/
    complexity_variables=0.;/*set complexity variable to 0*/
    unsigned int _size_init=static_cast<unsigned int>(_list_fixed_element.size());
    for (unsigned int i=_size_init; i<_N; ++i) {
        ptrV[i]->compute_s();/*compute surveys for each variable node*/
        if(g_T_cav>0.) {/*site term of Sigma_e = G + y e (see update_complexity_clauses)*/
            complexity_variables-=log(ptrV[i]->complexity_variable)+ptrV[i]->energy_variable/g_T_cav;
        } else {
            complexity_variables+=(static_cast<double>(ptrV[i]->_degree_i)-1.)*log(ptrV[i]->complexity_variable);/*update graph variable complexity*/
        }
    }
    if (g_dynamic_i || g_fe_backtrack) {/*release scores of the fixed variables at this fixed point*/
        for (unsigned int i=0; i<_size_init; ++i) ptrV[i]->_Ik=release_I(ptrV[i]);
    }
    complexity=complexity_clauses-complexity_variables;/*compute graph total complexity*/
    if(_numb_of_dec_moves==1 and _numb_of_back_moves==1) _comp_init=complexity;
    /*Paramagnetic phase: no variable of degree >= 2 can receive warnings in both
     directions, so every hard site factor 1-pi^+pi^- is 1. This is the legacy
     test complexity_variables==0, written on the hard factors so that it also
     holds with --rsb-gamma, where a warning-free site has z=e^gamma.*/
    bool _para=true;
    for (unsigned int i=_size_init; i<_N && _para; ++i) {
        Vertex *v=ptrV[i];
        if (v->_degree_i>=2 && v->_p_IND()+v->_p_MINUS()+v->_p_PLUS()!=1.) _para=false;
    }
    if(_para) complexity=0;
    /*Variant 3: keep the 2016 ratio, but spend extra releases when the last
     step removed a large fraction of Sigma or SP took many iterations.
     The total loss between two endpoints does not rank paths; the slope does.*/
    double r_use=g_r_bsp;
    if (g_adaptive_r && _have_sigma_prev) {
        double drop=_sigma_prev-complexity;
        /*Ignore the last digits. A relative drop is a cliff only while Sigma
         is still a real fraction of its initial value.*/
        bool steep=(_sigma_prev>0.01*_comp_init) && (drop>k_slope_frac*_sigma_prev);
        bool slow=_time_conv_print>k_eta_cut;
        if (steep || slow) {
            r_use=g_r_bsp+k_r_boost;
            if (r_use>0.99) r_use=0.99;
            BSP_INFO<<"adaptive backtrack r="<<r_use
                    <<" steep="<<(steep?1:0)<<" slow="<<(slow?1:0)
                    <<" dSigma="<<drop<<" eta="<<_time_conv_print<<endl;
        }
    }
    _sigma_prev=complexity;
    _have_sigma_prev=true;
    if(g_fe_backtrack) {/*ThermoSP: Gibbs split between a release and a decimation*/
        unsigned int _n_fixed=static_cast<unsigned int>(_list_fixed_element.size());
        if(_n_fixed==0) {/*nothing to release before the first decimation*/
            fl_bsp=false;
            ++_numb_of_dec_moves;
        } else {
            double _min_best=1e300;/*best (smallest) min_sigma Phi over the free variables*/
            for(unsigned int i=_n_fixed; i<_N; ++i) {
                double _best=(ptrV[i]->_phi_plus<ptrV[i]->_phi_minus)?ptrV[i]->_phi_plus:ptrV[i]->_phi_minus;
                if(_best<_min_best)_min_best=_best;
            }
            double _q_bt=-1e300;/*value of the best release, DeltaPhi minus the cost*/
            for(unsigned int i=0; i<_n_fixed; ++i) {
                if(ptrV[i]->_forced_by_up)continue;/*UP releases are futile: UP re-forces them*/
                double _gain=-g_T_cav*log(ptrV[i]->_Ik)-g_bt_cost;
                if(_gain>_q_bt)_q_bt=_gain;
            }
            if(_q_bt==-1e300) {/*no release candidate: only a decimation is possible*/
                fl_bsp=false;
                ++_numb_of_dec_moves;
            } else {
                /*P(backtrack)=e^(Q_bt/T)/(e^(Q_bt/T)+e^(Q_dec/T)) with Q_dec=-min_best*/
                fl_bsp=thermo_gibbs_direction(-_min_best,_q_bt,g_T_act);
                if(fl_bsp)++_numb_of_back_moves;
                else ++_numb_of_dec_moves;
            }
        }
    } else if((_numb_of_back_moves/_numb_of_dec_moves)<r_use) {
        ++_numb_of_back_moves;
        fl_bsp=true;/*update values for BSP ratio choice.*/
        //if(complexity<1.e-6 and complexity>0)fl_bsp=false; /*The algorithm is close to call walksat, and for safety reasons it makes only decimations*/
        /*this checks can be removed, because does not affect the algorithm*/
    } else {
        ++_numb_of_dec_moves;
        fl_bsp=false;
    }
}

/*public member class Graph. Logs one SP fixed point for Phase 1 diagnostics.
 No-op unless --diag=PREFIX was given. Called after surveys(), so fl_bsp
 already holds this step's decimation-vs-backtracking decision. Fixed
 variables are logged with their stale surveys plus fixed=1.*/
void Graph::diag_step() {
    if (g_diag_prefix.empty()) return;
    if (!_diag_header_done) {
        if (_N > 20000 && g_diag_every < 10)
            BSP_WARN<<"--diag-every="<<g_diag_every<<" with N="<<_N
                    <<": per-variable log will be huge"<<endl;
        _diag_step_out.open((g_diag_prefix + "_steps.csv").c_str());
        _diag_var_out.open((g_diag_prefix + "_vars.csv").c_str());
        _diag_move_out.open((g_diag_prefix + "_moves.csv").c_str());
        if (!_diag_step_out || !_diag_var_out || !_diag_move_out) {
            BSP_ERROR<<"Cannot open diagnostic output with prefix "<<g_diag_prefix<<endl;
            exit(-1);
        }
        _diag_step_out<<"# scorer="<<bsp_scorer_name()<<" K="<<_K<<" N="<<_N
                      <<" M="<<_M<<" seed="<<_seed<<" r="<<g_r_bsp
                      <<" theta="<<g_bsp_theta<<" veto="<<g_veto
                      <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _diag_step_out<<"step,move,Nt,Mt,Sigma,Sigma_per_N,eta,unit_prop,last_cert,n_fixed,sp_sweeps\n";
        _diag_var_out<<"# scorer="<<bsp_scorer_name()<<" K="<<_K<<" N="<<_N
                     <<" M="<<_M<<" seed="<<_seed<<" r="<<g_r_bsp
                      <<" theta="<<g_bsp_theta<<" veto="<<g_veto
                      <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _diag_var_out<<"step,vertex,fixed,who,sT,sF,sI,score,abspol,degree";
        if(g_T_cav>0.)_diag_var_out<<",e_mean,log_z";/*ThermoSP cavity statistics*/
        _diag_var_out<<"\n";
        _diag_move_out<<"# scorer="<<bsp_scorer_name()<<" K="<<_K<<" N="<<_N
                      <<" M="<<_M<<" seed="<<_seed<<" r="<<g_r_bsp
                      <<" theta="<<g_bsp_theta<<" veto="<<g_veto
                      <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _diag_move_out<<"step,action,vertex,dir";
        if(g_T_cav>0.)_diag_move_out<<",e_mean,log_z";/*ThermoSP cavity statistics*/
        _diag_move_out<<"\n";
        _diag_header_done=true;
    }
    unsigned step=_diag_step_idx++;
    _diag_step_out<<step<<","<<(fl_bsp ? "back" : "dec")<<","
                  <<_N_t<<","<<_M_t<<","
                  <<setprecision(10)<<complexity<<","
                  <<(complexity/static_cast<double>(_N))<<","
                  <<_time_conv_print<<","<<_unit_prop<<","
                  <<_last_certitude<<","<<_list_fixed_element.size()<<","
                  <<_sp_sweeps<<endl;
    if (step % g_diag_every != 0) return;
    for (unsigned i=0; i<_N; ++i) {
        Vertex *v=ptrV[i];
        _diag_var_out<<step<<","<<v->_vertex<<","
                     <<(v->_I_am_a_fixed_variable ? 1 : 0)<<","
                     <<(v->_who_I_am ? 1 : 0)<<","
                     <<setprecision(10)<<v->_sT<<","<<v->_sF<<","<<v->_sI<<","
                     <<v->_sC<<","<<fabs(v->_sT-v->_sF)<<","
                     <<v->_degree_i;
        if(g_T_cav>0.) {/*ThermoSP cavity statistics of the last free state*/
            ThermoStar _st=v->cavity_star(true,NULL,v->prod_V_plus,v->prod_V_minus,g_T_cav,g_gamma_eff);
            _diag_var_out<<","<<_st.e_mean<<","<<log(_st.z);
        }
        _diag_var_out<<"\n";
    }
    _diag_var_out<<flush;
    if (g_dump_residuals) {
        ostringstream name;
        name<<g_diag_prefix<<"_res_s"<<step<<".cnf";
        print_residual_to(name.str());
    }
}

/*public member class Graph. Logs one variable fix (dec/up) or release (back)
 for Phase 1 diagnostics. dir is 1/0 for fixes, -1 for releases. The step is
 _diag_step_idx-1 because moves execute after their SP fixed point was logged.
 Pre-SP unit propagations are skipped (headers not written yet).*/
void Graph::diag_move(const char* action, Vertex* v, int dir) {
    if (_in_trial) return;/*trial children must not touch shared file offsets*/
    if (g_diag_prefix.empty() || !_diag_header_done) return;
    _diag_move_out<<(_diag_step_idx-1)<<","<<action<<","<<v->_vertex<<","<<dir;
    if(g_T_cav>0.) {/*ThermoSP cavity statistics of the last free state*/
        ThermoStar _st=v->cavity_star(true,NULL,v->prod_V_plus,v->prod_V_minus,g_T_cav,g_gamma_eff);
        _diag_move_out<<","<<_st.e_mean<<","<<log(_st.z);
    }
    _diag_move_out<<endl;
}

#ifndef _WIN32
/*SIGALRM plumbing for bounding minisat (minisat_check callers).*/
static volatile sig_atomic_t s_oracle_alarm = 0;
static void oracle_alarm_handler(int) {
    s_oracle_alarm = 1;
}
#endif

/*public member class Graph. Runs minisat on a residual CNF file, bounded by
 g_oracle_timeout seconds. Returns 1 SAT, 0 UNSAT, -2 timeout, -3 fork
 failure/crash, -4 minisat binary missing. POSIX only.*/
int Graph::minisat_check(const string& path) {
#ifdef _WIN32
    (void)path;
    return -3;
#else
    pid_t gpid=fork();
    if (gpid < 0) return -3;
    if (gpid==0) {
        int dn=open("/dev/null", O_WRONLY);
        if (dn>=0) { dup2(dn, STDOUT_FILENO); dup2(dn, STDERR_FILENO); }
        execlp(g_minisat_path.c_str(), "minisat",
               path.c_str(), "/dev/null", (char*)NULL);
        _exit(127);/*exec failed: minisat missing*/
    }
    int gst=0;
    bool have_status=false;
    struct sigaction sa_new, sa_old;
    bool sa_saved=false;
    if (g_oracle_timeout > 0) {
        s_oracle_alarm=0;
        sa_new.sa_handler=oracle_alarm_handler;
        sigemptyset(&sa_new.sa_mask);
        sa_new.sa_flags=0;/*no SA_RESTART: waitpid must EINTR*/
        sigaction(SIGALRM, &sa_new, &sa_old);
        sa_saved=true;
        alarm(g_oracle_timeout);
    }
    while (true) {
        pid_t w=waitpid(gpid, &gst, 0);
        if (w==gpid) { have_status=true; break; }
        if (errno==EINTR && !s_oracle_alarm) continue;/*stray signal*/
        break;/*timeout (alarm) or hard error*/
    }
    if (g_oracle_timeout > 0) {
        alarm(0);
        if (sa_saved) sigaction(SIGALRM, &sa_old, NULL);
    }
    if (!have_status) {
        kill(gpid, SIGKILL);
        int dummy=0;
        while (waitpid(gpid, &dummy, 0) < 0 && errno==EINTR) { /*retry*/ }
        return -2;
    }
    if (WIFEXITED(gst) && WEXITSTATUS(gst)==127) return -4;
    if (WIFEXITED(gst) && (WEXITSTATUS(gst)==10 || WEXITSTATUS(gst)==20))
        return (WEXITSTATUS(gst)==10) ? 1 : 0;
    return -3;
#endif
}

/*public member class Graph. Tentatively fixes v to dir, minisat-checks the
 residual, then undoes the fix exactly (no SP runs between, so clean/build
 are perfectly symmetric). Returns minisat_check's code. Parent-side only.*/
int Graph::oracle_try(Vertex* v, bool dir) {
    _list_fixed_element.push_back(v);
    v->_it_list_fixed_elem=--_list_fixed_element.end();
    v->_I_am_a_fixed_variable=true;
    v->_who_I_am=dir;
    clean(v);
    v->_degree_i=0;
    unsigned mt_save=_M_t;
    _M_t=static_cast<unsigned>(_cl_list.size());
    ostringstream tmp;
    tmp<<"oracle_try_"<<(int)getpid()<<"_"<<(_oracle_tmp_ctr++)<<".cnf";
    print_residual_to(tmp.str());
    int r=minisat_check(tmp.str());
    unlink(tmp.str().c_str());
    _M_t=mt_save;
    _list_fixed_element.erase(v->_it_list_fixed_elem);
    v->_it_list_fixed_elem=_list_fixed_element.end();
    v->reset_value_default_var_i();
    build(v);
    return r;
}

/*public member class Graph. Exact direction resolution: minisat-check both
 residual outcomes, take a SAT direction (SP-preferred on ties). Returns
 -1 to use the SP rule when both agree or the oracle is unusable.*/
int Graph::oracle_dir(Vertex* v) {
    bool sp=v->_sT > v->_sF;
    int r_sp=oracle_try(v, sp);
    if (r_sp==-4) {
        BSP_ERROR<<"minisat not found at '"<<g_minisat_path<<"'"<<endl;
        exit(-1);
    }
    note_oracle_result(r_sp);
    int r_flip=oracle_try(v, !sp);
    if (r_flip==-4) {
        BSP_ERROR<<"minisat not found at '"<<g_minisat_path<<"'"<<endl;
        exit(-1);
    }
    note_oracle_result(r_flip);
    bool sp_sat=(r_sp==1), flip_sat=(r_flip==1);
    if (sp_sat && !flip_sat) return sp ? 1 : 0;
    if (flip_sat && !sp_sat) return sp ? 0 : 1;
    return -1;
}

/*public member class Graph. Timeout accounting for oracle-guided decimation:
 auto-disable after 3 consecutive or 10 total minisat timeouts.*/
void Graph::note_oracle_result(int r) {
    if (r==-2) {
        ++_oracle_timeouts;
        ++_oracle_timeouts_total;
        if (!_oracle_disabled &&
            (_oracle_timeouts>=3 || _oracle_timeouts_total>=10)) {
            _oracle_disabled=true;
            BSP_WARN<<"oracle auto-disabled after repeated minisat timeouts"<<endl;
        }
    } else _oracle_timeouts=0;
}


void Graph::unit_propagation() {
    unsigned long __k;
    unsigned int C;
    /*unit propagation*/
    for(__k=_m; __k<_M; __k++) {
        C=(vec_list_cl[__k])->_c;
        if(_cl[C]._I_am_in_list_unsat and _cl[C].empty()) { /*check if it is empty*/
            BSP_ERROR<<"Contradiction found"<<endl; /*contradiction found*/
            BSP_ERROR<<"I quit from convergence_messages function"<<endl;
            exit(-1);/*exit failure*/
        }
        if(_cl[C].size()==1) {
            /*set the variable to 1 otherwise you will have contradiction*/
            unit_propagation(C); /*fix the variable into clause c to true and clean the graph*/
            _counter_conv=0;/*set to zero variable counter convergence*/
            complexity_clauses=0.;/*set to zero clause complexity*/
            __k=_m-1;
        }
    }
    for(__k=_m; __k<_M; __k++) {
        C=(vec_list_cl[__k])->_c;
        if(_cl[C].size()==1) {
            BSP_WARN<<"there is an unit propagation not found"<<endl;
        }
    }
    _unit_prop=0;
    BSP_INFO<<"END UNIT PROPAGATION"<<endl;
}

/*public member class Graph. This member update all messages from clauses to variables and stops if:
 a) a convergence is found : SUCCESS;
 b) a contradiction is found : exit FAILURE;
 c) no convergence is found after t_max iteration: exit FAILURE
 */
void Graph::convergence_messages() { /*compute convergence messages for message passing algorithm*/
    bool conv_f=false;
    unsigned long i,l, __k;
    unsigned int C;
START:
    /*unit propagation*/
    for(__k=_m; __k<_M; __k++) {
        C=(vec_list_cl[__k])->_c;
        if(_cl[C]._I_am_in_list_unsat and _cl[C].empty()) { /*check if it is empty*/
            BSP_ERROR<<"Contradiction found"<<endl; /*contradiction found*/
            BSP_ERROR<<"I quit from convergence_messages function"<<endl;
            exit(-1);/*exit failure*/
        }
        if(_cl[C]._I_am_in_list_unsat and _cl[C].size()==1) {
            /*set the variable to 1 otherwise you will have contradiction*/
            unit_propagation(C); /*fix the variable into clause c to true and clean the graph*/
            _counter_conv=0;/*set to zero variable counter convergence*/
            complexity_clauses=0.;/*set to zero clause complexity*/
            __k=_m-1;
        }
    }
    update_products();
    /*convergence*/
    for (unsigned int t=0; t<t_max; ++t) {
        conv_f=false;
        _counter_conv=0;/*set counter convergence to zero*/
        complexity_clauses=0.;/*set clauses complexity to zero*/
        i=0;
        l=0;
        for(__k=_m; __k<_M; ++__k) { /*for on clause objects*/
            C=(vec_list_cl[__k])->_c;
            s=_cl[C]._size_cl_init;
            i=0;
            while (1) {
                if (_cl[C]._go_forward[i]) {
                    _new=1.;
                    norm=1.;
                    l=0;
                    while (1) {
                        if(i!=l && _cl[C]._go_forward[l]) {
                            _prod_S=_Pr_S(_cl[C].v_V[l],_cl[C].v_lit[l], _cl[C].div_s[l]);
                            _prod_U=_Pr_U(_cl[C].v_V[l],_cl[C].v_lit[l]);
                            if(g_T_cav>0. || g_gamma_eff!=0.) {/*ThermoSP or unfrozen bias*/
                                ThermoStar _st=_cl[C].v_V[l]->cavity_star(_cl[C].v_lit[l],_cl[C].v_survey_cl_to_i[l],_prod_S,_prod_U,g_T_cav,g_gamma_eff);
                                _new*=_st.pi_u;
                                norm*=_st.z;
                            } else {
                                _new*=__pu();/*new message from cl to variable is computed*/
                                norm*=__norm();
                            }
                        }
                        ++l;
                        if(l==s)break;
                    }
                    _new=compute_message(_new,norm);/*set to 0 a message iff the message is smaller than 1e-16*/
                    if(_new==1.) {
                        unit_propagation(C); /*fix the variable into clause c to true and clean the graph*/
                        BSP_WARN<<"The Instance is not really random, I have a survey equal to 1, which gives me nan."<<endl;
                        BSP_WARN<<"I try to use unit propagation and fix the variable to the best value for satisfying the clause"<<endl;
                        BSP_WARN<<"This may bring us to a contradiction"<<endl;
                        BSP_DEBUG<<"UP"<<endl;
                        goto START;
                    }
                    _cl[C].old_s[i]=_cl[C].update[i];
                    _cl[C].update[i]=_new+(_cl[C].old_s[i]-_new)*g_damping;/*damped update, legacy when 0*/
                    if(_counter_conv==0) {
                        if(_conv(_cl[C].update[i], _cl[C].old_s[i])) {
                            ++_counter_conv;
                        }
                    }
                    ++i;
                } else {
                    ++i;
                }
                if(i==s)break;
            }
        i=0;
        while (1) {
            double _eta=_cl[C].update[i];
            *_cl[C].v_survey_cl_to_i[i]=_eta;
            _cl[C].div_s[i]=Div_s(_eta);
            ++i;
            if(i==s)break;
        }
        }
        update_products();/*update products into vertex node for speeding up the algorithm*/
        if(_counter_conv==0) { /*if counter convergence is zero, a convergence is found*/
            _M_t=static_cast<unsigned int>(_cl_list.size());
            // cout<<"I found a convergence at "<<t<<" "<<_m<<endl;
            _time_conv_print=t;
            _sp_sweeps+=(unsigned long)t+1UL;
            update_complexity_clauses();/*update complexity_clause*/
            conv_f=true;
            break;
        }
    }
    if(!conv_f) { /*if after t_max iterations no convergence is found, the algorithm return exit failure */
        _sp_sweeps+=t_max;
        BSP_ERROR<<"SP does not find any fixed points -> SP does not converge."<<endl;
        BSP_ERROR<<"sp_sweeps="<<_sp_sweeps<<endl;
        BSP_ERROR<<"I am sorry I quit :("<<endl;
        exit(-1);
    }
}

/*public member class Graph. This member  updates only clause complexity.
 With g_T_cav>0 it gives the clause and edge part of the complexity
 Sigma_e = G + y e of the SP-y Bethe functional (paper/sections/spy.tex), with
 y=1/T_cav, G = sum_a F_a + sum_i F_i - sum_(i,a) F_ia and e = -dG/dy:
   F_a  = log[1-(1-e^-y) prod_j nu_{j->a}]
   F_ia = log[1-(1-e^-y) nu_{i->a} eta_{a->i}]
 The site part F_i + y e_i is in surveys().*/
void Graph::update_complexity_clauses() {
    double _ps=1.,_pu=1.;/*products for clause complexity*/
    unsigned long j;
    unsigned int C;
    //cout<<"This is the value where I start: "<<_M-_m<<endl;
    if(g_T_cav>0.) {
        const double y=1./g_T_cav, ey=exp(-y);
        for(unsigned int __k=_m; __k<_M; ++__k) {
            C=(vec_list_cl[__k])->_c;
            double prod_nu=1., edges=0.;
            for (j=0; j<_cl[C]._size_cl_init; ++j) {
                if(_cl[C]._go_forward[j]!=1) continue;
                _prod_S=_Pr_S(_cl[C].v_V[j],_cl[C].v_lit[j], _cl[C].div_s[j]);
                _prod_U=_Pr_U(_cl[C].v_V[j],_cl[C].v_lit[j]);
                ThermoStar _st=_cl[C].v_V[j]->cavity_star(_cl[C].v_lit[j],_cl[C].v_survey_cl_to_i[j],_prod_S,_prod_U,g_T_cav,g_gamma_eff);
                double nu=_st.pi_u/_st.z;
                double x=nu*(*_cl[C].v_survey_cl_to_i[j]);
                double d=1.-(1.-ey)*x;
                edges+=log(d)+y*ey*x/d;
                prod_nu*=nu;
            }
            double d=1.-(1.-ey)*prod_nu;
            complexity_clauses+=log(d)+y*ey*prod_nu/d-edges;
        }
        return;
    }
    for(unsigned int __k=_m; __k<_M; ++__k) {
        C=(vec_list_cl[__k])->_c;
        _ps=1.;/*initialize to one product __ps*/
        _pu=1.;/*initialize to one product __pu*/
        /*updating complexity clauses*/
        for (j=0; j<_cl[C]._size_cl_init; ++j) { /*for each literal in a clause clauses complexity is updated */
            if(_cl[C]._go_forward[j]==1) {
                _prod_S=_Pr_S(_cl[C].v_V[j],_cl[C].v_lit[j], _cl[C].div_s[j]);
                _prod_U=_Pr_U(_cl[C].v_V[j],_cl[C].v_lit[j]);
                if(g_T_cav>0. || g_gamma_eff!=0.) {/*same star sums as the message update*/
                    ThermoStar _st=_cl[C].v_V[j]->cavity_star(_cl[C].v_lit[j],_cl[C].v_survey_cl_to_i[j],_prod_S,_prod_U,g_T_cav,g_gamma_eff);
                    _ps*=_st.z;/*update product for clause complexity*/
                    _pu*=_st.pi_u;/*update product for clause complexity*/
                } else {
                    _ps*=__norm();/*update product for clause complexity*/
                    _pu*=__pu();/*update product for clause complexity*/
                }
            }
        }
        complexity_clauses+=log(_ps-_pu);/*update clause complexity*/
    }

}

/*public member class Graph which splits the global factor graph information in different objects and vectors.*/
void Graph::split_and_collect_information() { /*split the graph in different vectors and lists*/
    /*create clauses*/
    vec_list_cl.resize(_M);
    list<Vertex *>::iterator _it; /*iterator list of Vertex pointers*/
    double _rn=0.; /*random number*/
    unsigned int pos=0; /*position into a clause*/
    _cl[0].I_am_a_cl_true=false;/*set to false bool variable I_am_a_cl_true*/
    _cl_list.push_front(&_cl[0]);/*insert pointer of clause into the unsitisfied clauses list*/
    _cl[0]._it_list=_cl_list.begin();/*store the iterator of the unsitisfied clauses list*/
    _cl[0]._I_am_in_list_unsat=true;/*set to true the boolean varibale I_am_in_list_unsat*/
    vec_list_cl[0]=&_cl[0]; /*pointer to an element of a vector of Clause objects*/
    _m=0;
    for (unsigned int i=0, l=0; i<_ivec.size(); ++i) {
        if(_ivec[i]==0) {
            pos=0; /*set position to 0*/
            ++l; /*clause label increment*/
            if(l==_M)break;
            _cl[l].I_am_a_cl_true=false;/*set to false bool variable I_am_a_cl_true*/
            _cl_list.push_front(&_cl[l]);/*insert pointer of clause into the unsitisfied clauses list*/
            _cl[l]._it_list=_cl_list.begin();/*store the iterator of the unsitisfied clauses list*/
            _cl[l]._I_am_in_list_unsat=true;/*set to true the boolean varibale I_am_in_list_unsat*/
            vec_list_cl[l]=&_cl[l];
        } else {
            _cl[l]._c=l;/*clause labeling*/
            _cl[l]._l=l;/*position in vec_listy*/
            _cl[l].survey_cl_to_i_f_ptr.push_back(NULL); /*initialization vector of frozen survey pointers*/
            _cl[l].survey_cl_to_i_f.push_back(-1.);/*initialization vector of frozen surveys*/
            _cl[l]._vb.push_back(false); /*initialization Boolean vector*/
            _cl[l]._v_ref.push_back(-1);
            _cl[l].V.push_back(ptrV[abs(_ivec[i])-1]);/*Vertex pointer stored in list V*/
            _cl[l]._it_list_V_begin=_cl[l].V.begin();/*iterator stored into the clause*/
            _cl[l]._size_cl++; /*size of the clause*/
            _cl[l]._size_cl_init++; /*size of the clause at the beginning*/
            _cl[l].cpV.push_back(ptrV[abs(_ivec[i])-1]);/*copy of V in a vertex vector*/
            _cl[l]._vecpos.push_back(pos);/*vector of literal position in the clause*/
            _it=--_cl[l].V.end();/*pick iterator of V in cl*/
            ptrV[abs(_ivec[i])-1]->where_I_am.push_back(_it);/*store iterator*/
            ptrV[abs(_ivec[i])-1]->_degree_i++;/*update degree vertex i*/
            /*initialization surveys, real t, imag t-1*/
            _rn=random()/(RAND_MAX+1.0);/*choose a random number between 0-1*/
            ptrV[abs(_ivec[i])-1]->_surveys_cl_to_i.push_back(_rn);/*survey random initialization*/
            _cl[l].old_s.push_back(_rn);
            _cl[l].update.push_back(_rn);
            _cl[l].div_s.push_back(Div_s(_rn));
            pos++;/*update position*/
            if(_ivec[i]>0) { /*check if literal is negated or not*/
                _cl[l]._lit.push_back(true);/*store literal in Boolean list _lit*/
                _cl[l].cp_lit.push_back(true);/*store literal in a boolean vector*/
                _cl[l].cp_lit_int.push_back(1);/*store literal as integer*/
            } else {
                _cl[l]._lit.push_back(false);/*store literal in Boolean list _lit*/
                _cl[l].cp_lit.push_back(false);/*store literal in a boolean vector*/
                _cl[l].cp_lit_int.push_back(0);/*store literal as integer*/

            }
            ptrV[abs(_ivec[i])-1]->_lit_list_i.push_back(--_cl[l]._lit.end());/*store iterator in Vertex i*/
        }
    }
    /*To readers: please do not chenge this part, because we are using dynamic vectors and when one needs to store pointers of these vectors one needs to make safty choices. More precisely, each time that vector<T> constructor is called for allocating new memory, pointers can change and problems appear*/
    bool flag_s=true;
    _cl[0].v_survey_cl_to_i.resize(_cl[0].size());
    _cl[0]._go_forward.resize(_cl[0].size());
    _cl[0].v_lit.resize(_cl[0].size());
    _cl[0].v_V.resize(_cl[0].size());
    _cl[0]._var.resize(_cl[0].size());
    for (unsigned int i=0,k=0, l=0; i<_ivec.size(); ++i) {
        if(_ivec[i]==0) {
            k=0;
            ++l; /*increment of one clause label*/
            flag_s=true;
            if(l==_M)break;
            _cl[l]._go_forward.resize(_cl[l].size());
            _cl[l].v_survey_cl_to_i.resize(_cl[l].size());
            _cl[l].v_lit.resize(_cl[l].size());
            _cl[l].v_V.resize(_cl[l].size());
            _cl[l]._var.resize(_cl[l].size());
        } else {

            _cl[l].v_V[k]=ptrV[abs(_ivec[i])-1];/*Vertex pointer stored in list V*/
            _cl[l]._go_forward[k]=1;
            ptrV[abs(_ivec[i])-1]->_I_am_in_cl_at_init.push_back(complex<unsigned int *>(_cl[l]._ptr_c(),_cl[l]._ptr_pos(k)));/*store initial position of Vertex i into clause l*/
            ptrV[abs(_ivec[i])-1]->_bvec_lit.push_back(_cl[l]._ptr_lit_int(k));/*pointers vector literal variablee node*/
            _cl[l].survey_cl_to_i.push_back(ptrV[abs(_ivec[i])-1]->ptr_survey());/*store pointer of survey in cl l*/
            _cl[l].v_survey_cl_to_i[k]=ptrV[abs(_ivec[i])-1]->ptr_survey();
            _cl[l]._var[k]=_ivec[i];
            if(flag_s)_cl[l].survey_begin=_cl[l].survey_cl_to_i.begin();
            flag_s=false;
            if(_ivec[i]>0) {
                _cl[l].v_lit[k]=true;
                ptrV[abs(_ivec[i])-1]->_surveys_cl_to_i_plus.push_back(ptrV[abs(_ivec[i])-1]->ptr_survey());

            } else {
                _cl[l].v_lit[k]=false;
                ptrV[abs(_ivec[i])-1]->_surveys_cl_to_i_minus.push_back(ptrV[abs(_ivec[i])-1]->ptr_survey());
            }
            _cl[l].survey_cl_to_i_f_ptr[k++]=ptrV[abs(_ivec[i])-1]->ptr_survey();
            ptrV[abs(_ivec[i])-1]->_where_surveys_are_in_cl.push_back(--_cl[l].survey_cl_to_i.end());
            ptrV[abs(_ivec[i])-1]->_i++;
        }
    }

    update_products();/*update products*/
}

void Graph::update_products() { /*update products*/
    unsigned int _size_init=static_cast<unsigned int>(_list_fixed_element.size());
    for (unsigned int i=_size_init; i<_N; ++i) {
        //        if(!ptrV[i]->_I_am_a_fixed_variable){
        ptrV[i]->make_products();/*update products*/
        ptrV[i]->_i=0/*set variable _i in Vertex i to 0*/;
        //        }
    }
}

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/************************************ END SP ***************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/



/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***************************** START BACKTRACKING **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/

/*Sort the fixed prefix for a backtracking move. With --dynamic-i-backtrack
 or --fe-backtrack the release score is I(k), the fraction of clusters still
 compatible with the assigned value (see release_I). Smaller I(k) means the
 assignment kills more clusters, so it is released first. The sort is
 descending so that backtrack(), which releases from the end, picks the
 smallest I(k). Without the flags the 2016 stored bias is used.*/
void Graph::sort_V_Back_move() {
    unsigned nfixed = _list_fixed_element.size();
    if ((g_dynamic_i || g_fe_backtrack) && nfixed > 0) {
        /*_Ik comes from release_I at the current fixed point (see surveys()).
         The free-energy gain -T_cav log I(k) of --fe-backtrack has the same
         order. Variables fixed by unit propagation go to the front, because
         backtrack() stops at the first one it meets from the end.*/
        sort(ptrV.begin(), ptrV.begin() + nfixed,
             [](const Vertex* a, const Vertex* b) {
                 if (a->_forced_by_up != b->_forced_by_up) return a->_forced_by_up;
                 return a->_Ik > b->_Ik;
             });
    } else {
        sort(ptrV.begin(), ptrV.begin() + nfixed, _Vertex_greater_pred());
    }
}

/*I(k) = 1 - w_k^{-s_k} of a fixed variable k (eq. Ik of paper/sections/response.tex).
 The clauses satisfied by k or by any other variable send no warning. Each
 unsatisfied clause a contains k with a false literal and sends the warning
 eta_{a->k} = prod_j nu_{j->a} over its live literals j, computed from the
 current products. The stored message slots of k are not used: they keep the
 value of the moment when k was fixed. The unfrozen tilt gives the free mass
 B0 = prod_a (1-eta_{a->k}) the weight e^gamma.*/
double Graph::release_I(Vertex *k) {
    double B0=1.;
    for (unsigned j=0; j<k->_I_am_in_cl_at_init.size(); ++j) {
        Clause &c=_cl[*real(k->_I_am_in_cl_at_init[j])];
        if (!c._I_am_in_list_unsat) continue;
        double eta=1.;
        for (unsigned l=0; l<c._size_cl_init; ++l) {
            if (!c._go_forward[l]) continue;
            _prod_S=_Pr_S(c.v_V[l],c.v_lit[l],c.div_s[l]);
            _prod_U=_Pr_U(c.v_V[l],c.v_lit[l]);
            if (g_T_cav>0. || g_gamma_eff!=0.) {
                ThermoStar st=c.v_V[l]->cavity_star(c.v_lit[l],c.v_survey_cl_to_i[l],_prod_S,_prod_U,g_T_cav,g_gamma_eff);
                eta*=st.pi_u/st.z;
            } else {
                eta*=__pu()/__norm();
            }
        }
        B0*=1.-eta;
    }
    double free_mass=B0*exp(g_gamma_eff);
    double I=free_mass/(1.-B0+free_mass);
    return (I>1e-300)?I:1e-300;
}


/*public member calss Graph. This member builds up the clauses that are not anymore satisfied by the assignment of the variables*/
void Graph::build(Vertex * &V_to_build) {
    unsigned int k=0;
    unsigned int temp;
    for (unsigned int j=0; j<V_to_build->_surveys_cl_to_i.size(); ++j) {
        k=*real(V_to_build->_I_am_in_cl_at_init[j]);/*pick the clause that should be unfixed*/
        _cl[k].build(V_to_build, j);/*build the clause in the graph*/
        if(!_cl[k].I_am_a_cl_true and !_cl[k]._I_am_in_list_unsat) { /*check if the clasue is false and if it is in the list of un-sat clauses*/
            _cl[k]._I_am_in_list_unsat=true;
            _cl_list.push_front(&_cl[k]);/*set the clause into the list of un-sat clasues*/
            _cl[k]._it_list=_cl_list.begin();/*store the iterator of the list*/
            _M_t++;/*update index of un-sat clauses*/
            _m--; /*update index for fast calcualtion*/
            temp=_cl[k]._l;
            swap(vec_list_cl[_cl[k]._l], vec_list_cl[_m]);
            (vec_list_cl[_cl[k]._l])->_l=temp;
            (vec_list_cl[_m])->_l=_m;
        }
    }
}



/*public member class Graph. This member computes all operation for backtracking moves.*/
void Graph::backtrack() {
    //cout<<"I make a backtrack move"<<endl;
    sort_V_Back_move(); /*sort the elements in ptV vector, the ones in position 0, _m-1; in ascending order*/

    unsigned long _size=_m_t_m_1+_unit_prop;
    _unit_prop=0;
    /*NOTE: _m_t_m_1 intentionally keeps the last decimation batch size, so a
     backtrack following another backtrack still releases a full batch instead
     of (almost) nothing. A single backtrack after a decimation is unaffected.*/
    unsigned long _size_init=_list_fixed_element.size();
    if (_size > _size_init) _size = _size_init;
    for (unsigned long i=_size_init; i>_size_init-_size;) {
        if(!ptrV[--i]->_forced_by_up) {
            /*build the graph*/
            /*
             build the graph means:

             1) introducing into the factor graph all unsatisfied clauses, given by the fact that variable i is not anymore fixed;
             2) introducing literals associated to variable i, which are present in clauses not satisfied.
             */

            _list_fixed_element.erase(ptrV[i]->_it_list_fixed_elem);/*erase the variable node in fixed element list*/
            ptrV[i]->_it_list_fixed_elem = _list_fixed_element.end(); /*set the iterator to _list_fixed_element.end() by default*/
            diag_move("back", ptrV[i], -1);
            ptrV[i]->reset_value_default_var_i();/*reset values into the node*/
            build(ptrV[i]);/*build clauses*/

        } else {
            break;
        }

    }
    _N_t=_N-static_cast<unsigned int>(_list_fixed_element.size());/*update the number of un-fixed variable nodes*/
    //update_products();/*update products into vertex node for speeding up the algorithm*/

}

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/****************************** END BACKTRACKING  **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/

/*public member class Graph. This member prints the residual CNF formula to path.*/
void Graph::print_residual_to(const string& path) {
    ofstream outfile(path.c_str());
    if (!outfile) {
        BSP_ERROR<<"Error file output does not exist"<<endl;
        exit(-1);
    } else {
        outfile << "c seed=1234567"<<endl;
        outfile << "p cnf";
        outfile << ' ' << _N << ' ' << _M_t << endl;
        for (list<Clause *>::iterator i=_cl_list.begin(); i!=_cl_list.end(); ++i) {
            outfile<<_cl[(*i)->_c];/*print on file cluases*/
        }
    }
}

/*public member class Graph. This member prints on file the residual CNF formula.*/
void Graph::print_on_file_residual_formula() {
    ostringstream seed;
    seed<<_seed;/*real seed of the graph in the residual formula title*/
    const string s=seed.str();
    const string title="residualformula_seed";
    const string txt=".cnf";
    string rf=directory;
    rf+=title;
    rf+=s;
    rf+=txt;
    print_residual_to(rf);
}

/*public member class graph. This member calls WalkSAT function and computes the solution for the residual formula.
 Moreover, it builds the complete solution for the problem and also checks if all solutions are composed by frozen variables or not.*/
bool Graph::WalkSAT() {
    sort(ptrV.begin(), ptrV.end(), _Vertex_smaller_labeled_vertex_pred());/*simple ascending sort for labeled vertex.*/
    vector <vector<bool> > sol;
    ostringstream seed;
    seed<<_seed;
    const string s=seed.str();
    const string title="residualformula_seed";
    const string txt=".cnf";
    string rf=directory;
    myrand=1234567;/*seed for walksat*/
    rf+=title;
    rf+=s;
    rf+=txt;
    int argc=10;
    char * argv[10];
    for (int i=0; i<argc; i++) argv[i]=(char *)malloc(1000);
    strcpy (argv[0],"test" );
    strcpy (argv[1],"-solcnf" );
    strcpy (argv[2],"-cutoff");
    strcpy (argv[3],"100000000" );
    strcpy (argv[4],"-best" );
    strcpy (argv[5],"-seed" );
    strcpy (argv[6],"1234567");
    strcpy (argv[7],"-numsol" );
    strcpy (argv[8],"10" );
    strcpy (argv[9],rf.c_str());
    bool flag=false;
    for (int i=1; i<argc; i++) BSP_DEBUG<<"Input "<<argv[i]<<endl;

    WalkSat(sol,argc,argv);/*call WalkSAT*/
    for (int i=0; i<argc; i++) free(argv[i]);
    vector<long int> mysol;
    BSP_INFO<<"Check and white a solution"<<endl;
    BSP_INFO<<"time of whitening    freezing probability"<<endl;
    for (unsigned int i=0; i<sol.size(); i++) {
        if(!mysol.empty())mysol.clear();
        mysol.resize(_N+1);
        /*initialization procedure for whitening procedure*/
        for (unsigned int l=0; l<_M; ++l) {
            _cl[l].I_am_white=false;
        }
        for (unsigned int l=0; l<_N; ++l) {
            ptrV[l]->_I_am_white=false;
        }

        /*save SP solutions in vector mysol*/
        for (list<Vertex*>::iterator __it=_list_fixed_element.begin(); __it!=_list_fixed_element.end(); ++__it) {
            unsigned int var=(*__it)->_vertex;
            if((*__it)->_who_I_am)mysol[(*__it)->_vertex]=static_cast<long int>(var);
            else mysol[(*__it)->_vertex]=-(static_cast<long int>(var));
        }

        /*build a complete solution for the problem*/
        for (long int j=0; j<sol[i].size(); ++j) {
            if (!sol[i][j] and mysol[j+1]==0) {
                mysol[j+1]=-(j+1);
                ptrV[j]->_who_I_am=false;
                ptrV[j]->_I_am_a_fixed_variable=true;
            } else if(sol[i][j] and mysol[j+1]==0) {
                mysol[j+1]=(j+1);
                ptrV[j]->_who_I_am=true;
                ptrV[j]->_I_am_a_fixed_variable=true;
            }
        }

        /*check if the global solution is correct*/
        for(unsigned int c=0; c<_M; ++c) {  /*problem check*/
            if (!_cl[c]._check()) {
                BSP_ERROR<<_cl[c];
                BSP_ERROR<<"NO SOLUTION ERROR"<<endl;/*NO SOLUTION FOUND*/
                exit(-1);
            }
        }
        if(i==0)print_only_one_sol();
        if(!flag)flag=true;
        ostringstream alpha_str; //printing solution on file
        ostringstream var_str;
        ostringstream    i_str;
        ostringstream seed_str;
        string mystring;
        const string title="Solution_tot";
        const string seed="seed=";
        const string under="_";
        const string dat=".txt";

        alpha_str<<_alpha;
        var_str<<_N;
        i_str<<i;
        seed_str<<_seed;
        mystring=directory;
        mystring+=title;
        mystring+=alpha_str.str();
        mystring+=under;
        mystring+=var_str.str();
        mystring+=under;
        mystring+=i_str.str();
        mystring+=under;
        mystring+=seed;
        mystring+=seed_str.str();
        mystring+=dat;
        ofstream outfile5(mystring.c_str());
        for (int i=1; i<mysol.size(); ++i) {
            outfile5<<mysol[i]<<endl;
        }
        Whitening_Solution();/*whitening procedure on a complete solution*/
    }
    return flag;
}

/*Frozen and unfrozen variables and whitening procedure*/

/*The complexity of finding solutions close to SAT-UNSAT threshold could be correlated
 to the presence of frozen cluster. Frozen clusters are those clusters which contain an exponential
 number of frozen solutions. Frozen solutions are those solutions that contain frozen variables,
 and frozen variables are those variables that have the same values in all the solutions which in a cluster.
 Unforzen variables are those variables that can have different  values in the solutions
 that are in a forzen cluster. In other words, it is possible going from a frozen solution to another,
 both belonging to the same cluster, with a local rearangment of unfrozen variables.
 However, going from frozen solution to another solution belonging in different clusters
 needs a global rearrangment of all variables.
 For checking if a variable is frozen or not, and therefore if a solution is forzen or not,
 physicists have defined the whitening procedure, and for a SAT problem is described in the following way:

 start with a solution and assign iteratively a ”∗” (joker or white color) to variables which belong only
 to clauses which are already satisfied by another variable or already contain a "∗" variable.

 If a finite fraction of variable is not assigned to a "*" value, then all those variables are frozen variables
 and the solution  is called frozen. However, in our experience we have not met frozen solutions for K-SAT problems
 for N large enough. We have met them only for smaller values of N.

 */

/*public member class Graph.*/
void Graph::Whitening_Solution() { /*whitening procedure*/
    string mystring;/*string for printing file*/
    const string title="whitening";
    const string dat=".txt";
    ostringstream alpha_str;
    ostringstream var_str;

    alpha_str<<_alpha;
    var_str<<_N;
    mystring=directory;
    mystring+=title;
    mystring+=var_str.str();
    mystring+=alpha_str.str();
    mystring+=dat;
    ofstream outfilew(mystring.c_str(), ios_base::app );
    bool flag_white=true;/*Boolean flag for white*/
    double counter_white_var=0.;/*counter for white variables*/
    unsigned int time_white=0;/*time for whitening a solution*/
    for (unsigned int t=0; t<1024; ++t) {/*time loop*/
        time_white=t;
        for (unsigned int i=0; i<_N; ++i) {/*loop on all variable nodes*/
            if(!ptrV[i]->_I_am_white) {
                flag_white=true;
                ptrV[i]->_who_I_am=!ptrV[i]->_who_I_am;/*flip the variable and check if the new configuration is still a solution*/
                for (unsigned j=0; j<ptrV[i]->_I_am_in_cl_at_init.size(); ++j) {
                    if(!_cl[*real(ptrV[i]->_I_am_in_cl_at_init[j])].I_am_white) { /*check in all the clause where the variable appears*/
                        flag_white=flag_white and _cl[*real(ptrV[i]->_I_am_in_cl_at_init[j])]._check();
                        if(!flag_white)break;
                    }
                }
                if(flag_white) {
                    for (unsigned j=0; j<ptrV[i]->_I_am_in_cl_at_init.size(); ++j) {
                        if(!_cl[*real(ptrV[i]->_I_am_in_cl_at_init[j])].I_am_white) { /*if the variable is a "*" joker variable*/
                            _cl[*real(ptrV[i]->_I_am_in_cl_at_init[j])].I_am_white=true;/*clauses where joker variables appear become "white"*/
                        }
                    }
                    ptrV[i]->_I_am_white=true;
                    counter_white_var++;
                }
                ptrV[i]->_who_I_am=!ptrV[i]->_who_I_am;/*flip the variable to the correct value*/
                if(counter_white_var==static_cast<double>(_N)) {
                    t=1025;
                    break;
                }
            }
        }
        outfilew<<_seed<<" "<<_N<<" "<<_alpha<<" "<<time_white<<" "<<counter_white_var<<endl;/*print on file*/
    }
    BSP_INFO<<time_white<<"\t \t \t \t"<<counter_white_var/(double)(_N)<<endl;/*print on terminal*/
}

/*public member class Graph. This member prints the variable surveys at the first iteration*/
void Graph::print_only_one_sol() {
    /*output file*/
    const string title="Sol_SP_";
    const string txt=".txt";
    string str;
    ostringstream _seeds, _Ks, _sN, _salpha;
    _seeds<<_seed;
    _Ks<<_K;
    _salpha<<_alpha;
    _sN<<_N;
    str=directory;
    str=title;
    str+="_seed_";
    str+=_seeds.str();
    str+="K=";
    str+=_Ks.str();
    str+="N=";
    str+=_sN.str();
    str+="alpha=";
    str+=_salpha.str();
    str+=txt;
    ofstream outfilesol_(str.c_str(), ios_base::app);
    vector<long int> mysol;
    mysol.resize(_N+1);
    for (list<Vertex*>::iterator __it=_list_fixed_element.begin(); __it!=_list_fixed_element.end(); ++__it) {
        unsigned int var=(*__it)->_vertex;
        if((*__it)->_who_I_am)mysol[(*__it)->_vertex]=static_cast<long int>(var);
        else mysol[(*__it)->_vertex]=-(static_cast<long int>(var));
    }
    for (unsigned i=1; i<=_N; ++i) {
        if(mysol[i]!=0)outfilesol_<<mysol[i]<<" ";
    }
    outfilesol_<<_seed<<" "<<_comp_init/(double)_N<<" "<<endl;


}



/*public member class Graph. This member prints the variable surveys at the first iteration*/
void Graph::print_surveys() {
    /*output file*/
    const string title="Surveys";
    const string txt=".dat";
    string str;
    ostringstream _seeds, _Ks, _sN, _salpha;
    _seeds<<_seed;
    _Ks<<_K;
    _salpha<<_alpha;
    _sN<<_N;
    str=directory;
    str+=title;
    str+="_seed_";
    str+=_seeds.str();
    str+="K=";
    str+=_Ks.str();
    str+="N=";
    str+=_sN.str();
    str+="alpha=";
    str+=_salpha.str();
    //str+=sat;
    //str+=_seeds.str();
    //str+=_Ks.str();
    //str+=sat;
    //str+=_seeds.str();
    str+=txt;
    ofstream outfilewff(str.c_str(), ios_base::app);
    vector<long int> mysol;
    mysol.resize(_N+1);
    for (list<Vertex*>::iterator __it=_list_fixed_element.begin(); __it!=_list_fixed_element.end(); ++__it) {
        unsigned int var=(*__it)->_vertex;
        if((*__it)->_who_I_am)mysol[(*__it)->_vertex]=static_cast<long int>(var);
        else mysol[(*__it)->_vertex]=-(static_cast<long int>(var));
    }
    for (unsigned i=0; i<_N; ++i) {
        if(mysol[i+1]!=0)outfilewff<<_v_sT[i]<<" "<<_v_sI[i]<<" "<<_v_sF[i]<<" ";
    }
    outfilewff<<_seed<<" "<<_comp_init/(double)_N<<endl;


}
/*public member class Graph. This member prints the values on file*/
void Graph::print() {
    const string title="Value_Anal_Compl";
    const string txt=".txt";
    string str;
    ostringstream _seeds, _Ks, _sN, _salpha;
    _seeds<<_seed;
    _Ks<<_K;
    _salpha<<_alpha;
    _sN<<_N;
    str=directory;
    str=title;
    str+="_seed_";
    str+=_seeds.str();
    str+="K=";
    str+=_Ks.str();
    str+="N=";
    str+=_sN.str();
    str+="alpha=";
    str+=_salpha.str();
    //str+=sat;
    //str+=_seeds.str();
    //str+=_Ks.str();
    //str+=sat;
    //str+=_seeds.str();
    str+=txt;
    ofstream outfilecomp_(str.c_str(), ios_base::app);
    for (unsigned i=0; i<_v_Nt.size(); ++i) {
        outfilecomp_<<_v_Nt[i]<<" "<<_v_M_t[i]<<" "<<_v_c[i]<<" "<<_v_time[i]<<" "<<_N<<" "<<g_r_bsp<<endl;
    }

}

/*public member class Graph. In this member an instance of the problem is built and is printed on file*/
void Graph::write_on_file_graph() { /*build a graph and write a CNF formula*/
    /* get the values from INPUT and store them in the right place*/
    _N=static_cast<unsigned int>(stoul(_argv[_argc-1],nullptr,0));
    _alpha=stod(_argv[_argc-2],nullptr);
    double m=((double)_N*_alpha);
    _M=static_cast<unsigned int>(m);
    while(static_cast<double>(_M)<m) {
        if(((double)_M)==m)break;
        _M++;
    }
    _alpha=(double)_M/(double)_N;
    _K=static_cast<unsigned int>(stoul(_argv[_argc-3],nullptr,0));
    BSP_INFO<<setprecision(9)
           <<"I am going to build an instance with N: "<<_N<<" variables and "<<_M<<" clauses with a clause density equal to "<<_alpha<<endl;
    _ivec.reserve(_K*_M);
    _N_t=_N;
    _M_t=0;
    /*string for output file*/
    const string title="Formula_CNF";
    const string sat="-SAT_seed=";
    const string txt=".cnf";
    string str;
    ostringstream _seeds, _Ks, _sN, _salpha;
    _seeds<<_seed;
    _Ks<<_K;
    _salpha<<_alpha;
    _sN<<_N;
    str=directory;
    str+=title;
    str+="K=";
    str+=_Ks.str();
    str+="N=";
    str+=_sN.str();
    str+="alpha=";
    str+=_salpha.str();
    str+=sat;
    str+=_seeds.str();
    str+=txt;

    /*I am modifing how to write the output file*/
#ifdef PRINT_FORMULA_CNF
    /*output file*/
    ofstream outfilewff(str.c_str(), ios_base::app);
    outfilewff << "c seed="<<_seed<<endl;
    outfilewff << "p cnf";
    outfilewff << ' ' << _N << ' ' << _M << endl;
#endif
    /******************************************************/
    /******************************************************/
    /********************** START *************************/
    /******** Henry Kautz & Bart Selman makewff.c *********/
    /******************************************************/
    /******************************************************/
    /******************************************************/

    /*minimal modifiications for compatibility in c++*/

    int i, j, k;
    int lit;
    int cl[MAX_CLEN];
    bool dup;

    /*build the CNF instance*/
    for (i=0; i<_M; i++) {

        for (j=0; j<_K; j++) {

            do {


                lit =  static_cast<int>(random() % _N) + 1;


                dup = false;

                for (k=0; k<j; k++)

                    if (lit == cl[k]) dup = true;

            } while(dup);

            cl[j] = lit;

        }
        /* flip the literal*/
        for (j=0; j<_K; j++) {

            if (_flip()) cl[j] *= -1;

            _ivec.push_back(cl[j]);

        }
        _ivec.push_back(0);
        /******************************************************/
        /******************************************************/
        /******************************************************/
        /******** Henry Kautz & Bart Selman makewff.c *********/
        /*********************** END **************************/
        /******************************************************/
        /******************************************************/

        /* print on file*/
#ifdef PRINT_FORMULA_CNF
        for (j=0; j<_K; j++)outfilewff<<cl[j]<<" ";

        outfilewff<<"0"<<" "<<endl;
#endif
    }

}

/*public member class Graph. This member reads an instance of a problem from a file*/
void Graph::read_from_file_graph() { /*read an instance given as INPUT*/
    /*open file to read*/
    ifstream infile(_argv[_argc-1].c_str());
    if (!infile) {
        BSP_ERROR<<_argv[_argc-1]<<endl;
        BSP_ERROR<<"File not found"<<endl;
        BSP_ERROR<<"Please check if the name is correct or if the file exists."<<endl;
        exit(-1);
    } else {
        string ogg1;
        int ogg2, ogg3;
        bool flag=false;
        int var1;
        int _clen=0;/*current clause length, for _K inference*/
        /*read file and store variables in _ivec*/
        /* A CNF formula is composed by by +- numbers. 0 identifies the end of a clause*/
        while (!infile.eof()) {
            if(infile.eof())break;
            if (!flag) {
                infile>>ogg1;
                if (ogg1=="c") {
                    infile>>ogg1;
                    string str;
                    str=ogg1.substr(5);
                    _seed_out=atoi(str.c_str());
                }
                if (ogg1== "cnf") {
                    infile>>ogg2>>ogg3;
                    _N=ogg2;
                    _M=ogg3;
                    _N_t=_N;
                    _M_t=_M;
                    _ivec.reserve(_M*10);
                    flag=true;
                }
            }
            if (flag) {
                infile>>var1;
                if(!infile)break;/*eof or parse failure (e.g. appended 2nd formula): stop*/
                _ivec.push_back(var1);
                if (var1==0) {/*end of clause: track max length as _K*/
                    if (_clen>0 && static_cast<unsigned>(_clen)>_K) _K=static_cast<unsigned>(_clen);
                    _clen=0;
                } else _clen++;
            }
        }
        vector<long int>(_ivec).swap(_ivec);
        _alpha=static_cast<double>(_M)/static_cast<double>(_N);/*density from header*/
    }
}

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/******************************  END GRAPH CLASS  **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


