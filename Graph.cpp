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

#include "Graph.hpp"
#include "NnScorer.hpp"
#ifndef _WIN32
#include <sys/mman.h>
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
bool g_dynamic_I_backtrack = false;
bool g_lyapunov = false;
bool g_lyapunov_check = false;
long g_lyapunov_step = -1;
/*Phase 3 dataset options (see Header.h).*/
string g_dataset_prefix;
unsigned g_dataset_k = 50;
unsigned g_dataset_every = 1;
long g_dataset_step = -1;
bool g_oracle = false;
string g_minisat_path = "minisat";
unsigned g_oracle_timeout = 10;
bool g_oracle_dir = false;
unsigned g_oracle_pick = 0;
string g_nn_path;
bool g_nn_veto = false;
double g_nn_cutoff = 0.0;

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

/*Reconstruct the current cavity warning from one incident clause to a fixed
 variable. Active clauses contain the edge-excluding divisor. In a satisfied
 clause the live messages are zero, so neighbor products are already cavity
 products and need no divisor.*/
double Graph::warning_to_fixed(Vertex* v, unsigned edge) {
    unsigned c=*real(v->_I_am_in_cl_at_init[edge]);
    unsigned pos=*imag(v->_I_am_in_cl_at_init[edge]);
    Clause& cl=_cl[c];
    for (unsigned j=0; j<cl._size_cl_init; ++j)
        if (j!=pos && !cl._go_forward[j] && cl._vb[j])
            return 0.;/*another fixed literal satisfies the clause*/

    double warning=1.;
    double normalization=1.;
    for (unsigned j=0; j<cl._size_cl_init; ++j) {
        if (j==pos || !cl._go_forward[j]) continue;
        Vertex* u=cl.v_V[j];
        double prod_u=cl.v_lit[j] ? u->prod_V_minus : u->prod_V_plus;
        double divisor=cl._I_am_in_list_unsat ? cl.div_s[j] : 1.;
        double prod_s=(cl.v_lit[j] ? u->prod_V_plus : u->prod_V_minus)
                      *divisor;
        warning*=(1.-prod_u)*prod_s;
        normalization*=prod_s+prod_u-prod_s*prod_u;
    }
    if (normalization<=ZERO) return 1.;
    double eta=warning/normalization;
    if (eta<0.) return 0.;
    if (eta>1.) return 1.;
    return eta;
}

/*Parisi's current I(k): the fraction of current clusters retaining fixed
 variable k at its actual assigned value.*/
double Graph::current_fixation_factor(Vertex* v) {
    double prod_plus=1.,prod_minus=1.;
    for (unsigned edge=0; edge<v->_I_am_in_cl_at_init.size(); ++edge) {
        double one_minus=1.-warning_to_fixed(v, edge);
        if (*v->_bvec_lit[edge]) prod_plus*=one_minus;
        else prod_minus*=one_minus;
    }
    double z=prod_plus+prod_minus-prod_plus*prod_minus;
    if (z<=ZERO) return 0.;
    double factor=(v->_who_I_am ? prod_minus : prod_plus)/z;
    if (factor<0.) return 0.;
    if (factor>1.) return 1.;
    return factor;
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

/*public member class Graph which helps us to fix the variables that have the highest value of certitude*/
void Graph::choose_var_to_fix_and_clean() {
    /*set the rangee over variables unfixed are into vertex ptrV*/
    _M_t=0;
    unsigned int _size=(unsigned int)(frac*((double)_N_t))+(unsigned int)_list_fixed_element.size();
    unsigned int _size_init=(unsigned int)_list_fixed_element.size();
    /*set unit propagation counter to zero*/
    _unit_prop=0;
    /*condition for erasing only one variable at each convergence when (frac*((double)_N_t)<1*/
    if (_size<=_size_init) {
        _size=1+_size_init;
    }
    unsigned int _batch=_size-_size_init;/*decimation width of this move*/
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
        complexity_variables+=(static_cast<double>(ptrV[i]->_degree_i)-1.)*log(ptrV[i]->complexity_variable);/*update graph variable complexity*/
    }
    complexity=complexity_clauses-complexity_variables;/*compute graph total complexity*/
    if(_numb_of_dec_moves==1 and _numb_of_back_moves==1) _comp_init=complexity;
    if(complexity_variables==0.) complexity=0;
    if((_numb_of_back_moves/_numb_of_dec_moves)<g_r_bsp) {
        ++_numb_of_back_moves;
        fl_bsp=true;/*update values for BSP ratio choice.*/
        //if(complexity<1.e-6 and complexity>0)fl_bsp=false; /*The algorithm is close to call walksat, and for safety reasons it makes only decimations*/
        /*this checks can be removed, because does not affect the algorithm*/
    } else {
        ++_numb_of_dec_moves;
        fl_bsp=false;
    }
}

/*Directional derivative of one complete synchronous SP sweep. Clause
 messages are updated into arrays during a sweep, while vertex products are
 refreshed only at its end, so the implemented map is Jacobi despite the
 sequential clause loop.*/
void Graph::jacobian_vector_product(const vector<unsigned long>& offsets,
                                    const vector<double>& tangent,
                                    vector<double>& next) {
    next.assign(tangent.size(), 0.);
    vector<double> sum_plus(_N, 0.);
    vector<double> sum_minus(_N, 0.);
    vector<double> dprod_plus(_N, 0.);
    vector<double> dprod_minus(_N, 0.);

    for (unsigned ci=_m; ci<_M; ++ci) {
        unsigned c=vec_list_cl[ci]->_c;
        Clause& cl=_cl[c];
        for (unsigned j=0; j<cl._size_cl_init; ++j) {
            if (!cl._go_forward[j]) continue;
            Vertex* v=cl.v_V[j];
            unsigned label=v->_vertex-1;
            double scaled=tangent[offsets[c]+j]*cl.div_s[j];
            if (cl.v_lit[j]) sum_plus[label]+=scaled;
            else sum_minus[label]+=scaled;
        }
    }
    for (unsigned i=0; i<_N; ++i) {
        Vertex* v=ptrV[i];
        unsigned label=v->_vertex-1;
        dprod_plus[label]=-v->prod_V_plus*sum_plus[label];
        dprod_minus[label]=-v->prod_V_minus*sum_minus[label];
    }

    for (unsigned ci=_m; ci<_M; ++ci) {
        unsigned c=vec_list_cl[ci]->_c;
        Clause& cl=_cl[c];
        for (unsigned target=0; target<cl._size_cl_init; ++target) {
            if (!cl._go_forward[target]) continue;
            double numerator=1.;
            double denominator=1.;
            double dnumerator=0.;
            double ddenominator=0.;
            for (unsigned j=0; j<cl._size_cl_init; ++j) {
                if (j==target || !cl._go_forward[j]) continue;
                Vertex* v=cl.v_V[j];
                unsigned label=v->_vertex-1;
                bool positive=cl.v_lit[j];
                double prod_s=positive ? v->prod_V_plus : v->prod_V_minus;
                double dprod_s=positive ? dprod_plus[label] : dprod_minus[label];
                double prod_u=positive ? v->prod_V_minus : v->prod_V_plus;
                double dprod_u=positive ? dprod_minus[label] : dprod_plus[label];
                double divisor=cl.div_s[j];
                double edge_tangent=tangent[offsets[c]+j];
                double S=prod_s*divisor;
                double dS=dprod_s*divisor
                           +prod_s*edge_tangent*divisor*divisor;
                double U=prod_u;
                double dU=dprod_u;
                double num_factor=(1.-U)*S;
                double dnum_factor=(1.-U)*dS-S*dU;
                double den_factor=S+U-rho_SP*S*U;
                double dden_factor=dS+dU-rho_SP*(dS*U+S*dU);

                dnumerator=dnumerator*num_factor
                             +numerator*dnum_factor;
                numerator*=num_factor;
                ddenominator=ddenominator*den_factor
                               +denominator*dden_factor;
                denominator*=den_factor;
            }
            if (denominator<=0.) {
                next[offsets[c]+target]=0.;
                continue;
            }
            double value=numerator/denominator;
            double derivative=(dnumerator*denominator
                               -numerator*ddenominator)
                              /(denominator*denominator);
            if (value<ZERO) derivative=0.;/*same clamp as compute_message()*/
            next[offsets[c]+target]=(1.-g_damping)*derivative
                                    +g_damping*tangent[offsets[c]+target];
        }
    }
}

/*Evaluate one SP sweep from a packed active-message vector without touching
 graph storage. Used only to validate the analytic tangent map.*/
void Graph::message_sweep(const vector<unsigned long>& offsets,
                          const vector<double>& messages,
                          vector<double>& next) {
    next.assign(messages.size(), 0.);
    vector<double> prod_plus(_N, 1.);
    vector<double> prod_minus(_N, 1.);
    for (unsigned ci=_m; ci<_M; ++ci) {
        unsigned c=vec_list_cl[ci]->_c;
        Clause& cl=_cl[c];
        for (unsigned j=0; j<cl._size_cl_init; ++j) {
            if (!cl._go_forward[j]) continue;
            unsigned label=cl.v_V[j]->_vertex-1;
            double factor=1.-messages[offsets[c]+j];
            if (cl.v_lit[j]) prod_plus[label]*=factor;
            else prod_minus[label]*=factor;
        }
    }
    for (unsigned ci=_m; ci<_M; ++ci) {
        unsigned c=vec_list_cl[ci]->_c;
        Clause& cl=_cl[c];
        for (unsigned target=0; target<cl._size_cl_init; ++target) {
            if (!cl._go_forward[target]) continue;
            double numerator=1.;
            double denominator=1.;
            for (unsigned j=0; j<cl._size_cl_init; ++j) {
                if (j==target || !cl._go_forward[j]) continue;
                unsigned label=cl.v_V[j]->_vertex-1;
                bool positive=cl.v_lit[j];
                double divisor=1./(1.-messages[offsets[c]+j]);
                double S=(positive ? prod_plus[label] : prod_minus[label])
                         *divisor;
                double U=positive ? prod_minus[label] : prod_plus[label];
                numerator*=(1.-U)*S;
                denominator*=S+U-rho_SP*S*U;
            }
            double value=numerator/denominator;
            if (value<ZERO) value=0.;
            next[offsets[c]+target]=(1.-g_damping)*value
                                    +g_damping*messages[offsets[c]+target];
        }
    }
}

/*Benettin/power estimate of the largest Lyapunov exponent at the current SP
 fixed point. This reads messages and products but never mutates solver state.*/
void Graph::compute_lyapunov_at_fixed_point() {
    _lyapunov_jvp_error=0.;
    _lyapunov_incoming_participation.assign(_N, 0.);
    _lyapunov_neighborhood_participation.assign(_N, 0.);
    vector<unsigned long> offsets(_M+1, 0);
    for (unsigned c=0; c<_M; ++c)
        offsets[c+1]=offsets[c]+_cl[c]._size_cl_init;
    vector<double> tangent(offsets[_M], 0.);
    vector<double> next(offsets[_M], 0.);
    unsigned state=_seed^0x9e3779b9U;
    unsigned active=0;
    long double norm2=0.;
    for (unsigned ci=_m; ci<_M; ++ci) {
        unsigned c=vec_list_cl[ci]->_c;
        for (unsigned j=0; j<_cl[c]._size_cl_init; ++j) {
            if (!_cl[c]._go_forward[j]) continue;
            state=1664525U*state+1013904223U;
            double value=(static_cast<double>(state)+0.5)/2147483648.0-1.;
            tangent[offsets[c]+j]=value;
            norm2+=static_cast<long double>(value)*value;
            ++active;
        }
    }
    if (active==0 || norm2<=0.) {
        _lyapunov_rho=0.;
        _lyapunov_exponent=-HUGE_VAL;
        _lyapunov_iterations=0;
        return;
    }
    double norm=static_cast<double>(sqrtl(norm2));
    for (unsigned i=0; i<tangent.size(); ++i) tangent[i]/=norm;

    if (g_lyapunov_check) {
        vector<double> messages(offsets[_M], 0.);
        for (unsigned ci=_m; ci<_M; ++ci) {
            unsigned c=vec_list_cl[ci]->_c;
            for (unsigned j=0; j<_cl[c]._size_cl_init; ++j)
                if (_cl[c]._go_forward[j])
                    messages[offsets[c]+j]=_cl[c].update[j];
        }
        vector<double> analytic;
        vector<double> plus(messages.size());
        vector<double> minus(messages.size());
        vector<double> map_plus;
        vector<double> map_minus;
        jacobian_vector_product(offsets, tangent, analytic);
        const double steps[4]={1.e-4,1.e-5,1.e-6,1.e-7};
        double best_error=HUGE_VAL;
        for (unsigned h_index=0; h_index<4; ++h_index) {
            double h=steps[h_index];
            for (unsigned i=0; i<messages.size(); ++i) {
                plus[i]=messages[i]+h*tangent[i];
                minus[i]=messages[i]-h*tangent[i];
            }
            message_sweep(offsets, plus, map_plus);
            message_sweep(offsets, minus, map_minus);
            long double error2=0.;
            long double finite_difference2=0.;
            for (unsigned ci=_m; ci<_M; ++ci) {
                unsigned c=vec_list_cl[ci]->_c;
                for (unsigned j=0; j<_cl[c]._size_cl_init; ++j) {
                    if (!_cl[c]._go_forward[j]) continue;
                    unsigned index=offsets[c]+j;
                    double finite_difference=(map_plus[index]-map_minus[index])
                                             /(2.*h);
                    double error=analytic[index]-finite_difference;
                    error2+=static_cast<long double>(error)*error;
                    finite_difference2+=static_cast<long double>(
                        finite_difference)*finite_difference;
                }
            }
            if (finite_difference2<=0.) continue;
            double relative=static_cast<double>(
                sqrtl(error2/finite_difference2));
            if (relative<best_error) best_error=relative;
        }
        _lyapunov_jvp_error=best_error;
    }

    vector<double> growth;
    unsigned aligned_count=0;
    for (unsigned iteration=0; iteration<128; ++iteration) {
        jacobian_vector_product(offsets, tangent, next);
        norm2=0.;
        for (unsigned i=0; i<next.size(); ++i)
            norm2+=static_cast<long double>(next[i])*next[i];
        norm=static_cast<double>(sqrtl(norm2));
        _lyapunov_iterations=iteration+1;
        if (norm<=1.e-300) {
            _lyapunov_rho=0.;
            _lyapunov_exponent=-HUGE_VAL;
            return;
        }
        growth.push_back(log(norm));
        for (unsigned i=0; i<next.size(); ++i) next[i]/=norm;
        double alignment=0.;
        for (unsigned i=0; i<next.size(); ++i)
            alignment+=tangent[i]*next[i];
        if (iteration>=8 && 1.-fabs(alignment)<1.e-10) ++aligned_count;
        else aligned_count=0;
        tangent.swap(next);
        if (aligned_count>=4) break;
    }
    unsigned window=growth.size()<16 ? static_cast<unsigned>(growth.size()) : 16;
    double exponent=0.;
    for (unsigned i=growth.size()-window; i<growth.size(); ++i)
        exponent+=growth[i];
    _lyapunov_exponent=exponent/static_cast<double>(window);
    _lyapunov_rho=exp(_lyapunov_exponent);

    for (unsigned ci=_m; ci<_M; ++ci) {
        unsigned c=vec_list_cl[ci]->_c;
        Clause& cl=_cl[c];
        double clause_energy=0.;
        for (unsigned j=0; j<cl._size_cl_init; ++j)
            if (cl._go_forward[j]) {
                double value=tangent[offsets[c]+j];
                clause_energy+=value*value;
                unsigned label=cl.v_V[j]->_vertex-1;
                _lyapunov_incoming_participation[label]+=value*value;
            }
        for (unsigned j=0; j<cl._size_cl_init; ++j)
            if (cl._go_forward[j]) {
                unsigned label=cl.v_V[j]->_vertex-1;
                _lyapunov_neighborhood_participation[label]+=clause_energy;
            }
    }
}

/*Measure both the implemented epsilon-stopping state and, in a discarded
 fork, the tighter fixed point reached from it. The parent BSP trajectory is
 untouched by the refinement.*/
void Graph::compute_lyapunov() {
    if (g_lyapunov_step>=0 &&
        _diag_step_idx!=static_cast<unsigned>(g_lyapunov_step)) {
        _lyapunov_rho=0.;
        _lyapunov_exponent=-HUGE_VAL;
        _lyapunov_iterations=0;
        _tight_lyapunov_rho=0.;
        _tight_lyapunov_exponent=-HUGE_VAL;
        _tight_lyapunov_iterations=0;
        _tight_lyapunov_converged=false;
        _tight_complexity=0.;
        _lyapunov_jvp_error=0.;
        _lyapunov_incoming_participation.assign(_N, 0.);
        _lyapunov_neighborhood_participation.assign(_N, 0.);
        return;
    }
    compute_lyapunov_at_fixed_point();
    _tight_lyapunov_rho=0.;
    _tight_lyapunov_exponent=-HUGE_VAL;
    _tight_lyapunov_iterations=0;
    _tight_lyapunov_converged=false;
    _tight_complexity=0.;
#ifndef _WIN32
    double* result=(double*)mmap(NULL, 6*sizeof(double),
                                 PROT_READ|PROT_WRITE,
                                 MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (result==MAP_FAILED) {
        BSP_ERROR<<"mmap failed for Lyapunov refinement"<<endl;
        exit(-1);
    }
    for (unsigned i=0; i<6; ++i) result[i]=0.;
    _diag_step_out<<flush;
    _diag_var_out<<flush;
    _diag_move_out<<flush;
    cout<<flush;
    cerr<<flush;
    fflush(NULL);
    pid_t pid=fork();
    if (pid<0) {
        munmap(result, 6*sizeof(double));
        BSP_ERROR<<"fork failed for Lyapunov refinement"<<endl;
        exit(-1);
    }
    if (pid==0) {
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        _in_trial=true;
        unsigned fixed_before=static_cast<unsigned>(_list_fixed_element.size());
        g_epsilon=ZERO;
        convergence_messages(8*t_max);
        surveys();
        if (_list_fixed_element.size()!=fixed_before) _exit(42);
        compute_lyapunov_at_fixed_point();
        result[1]=_lyapunov_rho;
        result[2]=_lyapunov_exponent;
        result[3]=static_cast<double>(_lyapunov_iterations);
        result[4]=complexity;
        result[5]=static_cast<double>(_list_fixed_element.size());
        result[0]=1.;
        _exit(0);
    }
    int status=0;
    while (waitpid(pid, &status, 0)<0 && errno==EINTR) {}
    bool valid=WIFEXITED(status) && WEXITSTATUS(status)==0 && result[0]>0.
               && static_cast<unsigned>(result[5])==_list_fixed_element.size();
    if (valid) {
        _tight_lyapunov_converged=true;
        _tight_lyapunov_rho=result[1];
        _tight_lyapunov_exponent=result[2];
        _tight_lyapunov_iterations=static_cast<unsigned>(result[3]);
        _tight_complexity=result[4];
    }
    munmap(result, 6*sizeof(double));
#endif
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
                      <<" dynamic_I_backtrack="<<g_dynamic_I_backtrack
                      <<" lyapunov="<<g_lyapunov
                      <<" lyapunov_step="<<g_lyapunov_step
                      <<" lyapunov_check="<<g_lyapunov_check
                      <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _diag_step_out<<"step,move,Nt,Mt,Sigma,Sigma_per_N,eta,unit_prop,last_cert,n_fixed,"
                      <<"lyapunov_rho,lyapunov_exponent,lyapunov_iterations,"
                      <<"lyapunov_jvp_error,"
                      <<"tight_converged,tight_lyapunov_rho,"
                      <<"tight_lyapunov_exponent,tight_lyapunov_iterations,"
                      <<"tight_Sigma\n";
        _diag_var_out<<"# scorer="<<bsp_scorer_name()<<" K="<<_K<<" N="<<_N
                     <<" M="<<_M<<" seed="<<_seed<<" r="<<g_r_bsp
                      <<" theta="<<g_bsp_theta<<" veto="<<g_veto
                      <<" dynamic_I_backtrack="<<g_dynamic_I_backtrack
                      <<" lyapunov="<<g_lyapunov
                      <<" lyapunov_step="<<g_lyapunov_step
                      <<" lyapunov_check="<<g_lyapunov_check
                      <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _diag_var_out<<"step,vertex,fixed,who,sT,sF,sI,score,abspol,degree,sNN,"
                     <<"lyapunov_incoming,lyapunov_neighborhood\n";
        _diag_move_out<<"# scorer="<<bsp_scorer_name()<<" K="<<_K<<" N="<<_N
                      <<" M="<<_M<<" seed="<<_seed<<" r="<<g_r_bsp
                      <<" theta="<<g_bsp_theta<<" veto="<<g_veto
                      <<" dynamic_I_backtrack="<<g_dynamic_I_backtrack
                      <<" lyapunov="<<g_lyapunov
                      <<" lyapunov_step="<<g_lyapunov_step
                      <<" lyapunov_check="<<g_lyapunov_check
                      <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _diag_move_out<<"step,action,vertex,dir\n";
        _diag_header_done=true;
    }
    unsigned step=_diag_step_idx++;
    _diag_step_out<<step<<","<<(fl_bsp ? "back" : "dec")<<","
                  <<_N_t<<","<<_M_t<<","
                  <<setprecision(10)<<complexity<<","
                  <<(complexity/static_cast<double>(_N))<<","
                  <<_time_conv_print<<","<<_unit_prop<<","
                  <<_last_certitude<<","<<_list_fixed_element.size()<<","
                  <<_lyapunov_rho<<","<<_lyapunov_exponent<<","
                  <<_lyapunov_iterations<<","<<_lyapunov_jvp_error<<","
                  <<(_tight_lyapunov_converged ? 1 : 0)<<","
                  <<_tight_lyapunov_rho<<","<<_tight_lyapunov_exponent<<","
                  <<_tight_lyapunov_iterations<<","<<_tight_complexity<<endl;
    if (step % g_diag_every != 0) return;
    for (unsigned i=0; i<_N; ++i) {
        Vertex *v=ptrV[i];
        _diag_var_out<<step<<","<<v->_vertex<<","
                     <<(v->_I_am_a_fixed_variable ? 1 : 0)<<","
                     <<(v->_who_I_am ? 1 : 0)<<","
                     <<setprecision(10)<<v->_sT<<","<<v->_sF<<","<<v->_sI<<","
                     <<v->_sC<<","<<fabs(v->_sT-v->_sF)<<","
                     <<v->_degree_i<<","<<v->_sNN<<","
                     <<(_lyapunov_incoming_participation.size()==_N
                        ? _lyapunov_incoming_participation[v->_vertex-1] : 0.)<<","
                     <<(_lyapunov_neighborhood_participation.size()==_N
                        ? _lyapunov_neighborhood_participation[v->_vertex-1] : 0.)<<"\n";
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
    _diag_move_out<<(_diag_step_idx-1)<<","<<action<<","<<v->_vertex<<","<<dir<<endl;
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

/*public member class Graph. Timeout accounting shared by dataset trials
 and oracle-guided decimation: auto-disable after 3 consecutive or 10 total.*/
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

/*public member class Graph. Runs tentative-fix trials for the Phase 3
 DeltaSigma dataset. For each shortlisted unfixed variable and each direction,
 a forked child fixes it, optionally checks the trial residual with an exact
 SAT oracle (minisat), reconverges SP and reports through shared memory; the
 parent is undisturbed so no undo is needed. Labels per trial: oracle SAT flag
 (exact fatality gate: 1/0, -2 on oracle timeout), 1-step DeltaSigma
 (gradation), SP-converged flag.
 The oracle runs BEFORE SP so its result survives SP death in the child.
 Minisat is bounded by g_oracle_timeout seconds per trial and auto-disables
 for the run after repeated timeouts (K=4 near threshold can be
 exponentially hard for DPLL).
 No-op unless --dataset=PREFIX was given. POSIX only (fork/mmap).*/
void Graph::dataset_trials() {
    if (g_dataset_prefix.empty()) return;
#ifdef _WIN32
    BSP_ERROR<<"--dataset needs fork/mmap (POSIX); not supported on Windows"<<endl;
    exit(-1);
#else
    if (complexity==0. || _N_t==0) return;
    /*0-based SP-step index, aligned with diag_step()'s numbering for joins*/
    unsigned step=static_cast<unsigned>(_numb_of_dec_moves+_numb_of_back_moves-3.);
    unsigned diag_step=_diag_step_idx ? _diag_step_idx-1 : 0;
    if (g_dataset_step>=0) {
        if (diag_step!=static_cast<unsigned>(g_dataset_step)) return;
    } else if (step % g_dataset_every != 0) return;
    if (!_ds_header_done) {
        _ds_out.open((g_dataset_prefix + "_dataset.csv").c_str());
        if (!_ds_out) {
            BSP_ERROR<<"Cannot open dataset output with prefix "<<g_dataset_prefix<<endl;
            exit(-1);
        }
        _ds_out<<"# scorer="<<bsp_scorer_name()<<" K="<<_K<<" N="<<_N
                <<" M="<<_M<<" seed="<<_seed<<" r="<<g_r_bsp
                <<" theta="<<g_bsp_theta<<" veto="<<g_veto
                <<" dynamic_I_backtrack="<<g_dynamic_I_backtrack
                <<" lyapunov="<<g_lyapunov
                <<" lyapunov_step="<<g_lyapunov_step
                <<" lyapunov_check="<<g_lyapunov_check
                <<" dataset_step="<<g_dataset_step
                <<" eps="<<g_epsilon<<" damp="<<g_damping<<"\n";
        _ds_out<<"step,move,vertex,dir,sT,sF,sI,bias_cert,score,abspol,margin,"
               <<"prod_plus,prod_minus,degree,n_inc,len1,len2,len3,len4p,"
               <<"Sigma_before,Sigma_per_N,step_frac,eta_before,r,"
               <<"converged,crashed,sat_oracle,delta_sigma,eta_after,"
               <<"lyapunov_rho_after,lyapunov_exponent_after,"
               <<"tight_sigma_after\n";
        _ds_header_done=true;
    }
    /*shortlist: top-K unfixed by active score (index copy, ptrV untouched)*/
    vector<Vertex*> cand;
    cand.reserve(_N_t);
    for (unsigned i=0; i<_N; ++i)
        if (!ptrV[i]->_I_am_a_fixed_variable) cand.push_back(ptrV[i]);
    unsigned k = g_dataset_k < cand.size() ? g_dataset_k : (unsigned)cand.size();
    if (k==0) return;
    partial_sort(cand.begin(), cand.begin()+k, cand.end(), _Vertex_greater_pred());
    /*shared report: convergence, DeltaSigma, eta, SAT, rho, lambda, Sigma*/
    double* tout = (double*)mmap(NULL, 7*sizeof(double), PROT_READ|PROT_WRITE,
                                 MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (tout==MAP_FAILED) {
        BSP_ERROR<<"mmap failed for dataset trial"<<endl;
        exit(-1);
    }
    double sigma_before=complexity;
    const char* mv = fl_bsp ? "back" : "dec";
    for (unsigned ci=0; ci<k; ++ci) {
        Vertex* v=cand[ci];
        double sT=v->_sT, sF=v->_sF, sI=v->_sI;
        double bias=1.-(sT<sF?sT:sF);
        double abspol=fabs(sT-sF);
        double margin=(sT+sF>0.) ? abspol/(sT+sF) : 0.;
        unsigned n_inc=0,len1=0,len2=0,len3=0,len4p=0;
        for (unsigned j=0; j<v->_I_am_in_cl_at_init.size(); ++j) {
            unsigned c=*real(v->_I_am_in_cl_at_init[j]);
            if (!_cl[c]._I_am_in_list_unsat) continue;
            ++n_inc;
            unsigned L=(unsigned)_cl[c].size();
            if (L<=1) ++len1;
            else if (L==2) ++len2;
            else if (L==3) ++len3;
            else ++len4p;
        }
        ostringstream tail;
        tail<<setprecision(10)<<sT<<","<<sF<<","<<sI<<","<<bias<<","<<v->_sC<<","
            <<abspol<<","<<margin<<","<<v->prod_V_plus<<","<<v->prod_V_minus<<","
            <<v->_degree_i<<","<<n_inc<<","<<len1<<","<<len2<<","<<len3<<","<<len4p<<","
            <<sigma_before<<","<<(sigma_before/static_cast<double>(_N))<<","
            <<(static_cast<double>(_N-_N_t)/static_cast<double>(_N))<<","
            <<_time_conv_print<<","<<g_r_bsp;
        for (int dir=0; dir<=1; ++dir) {
            for (unsigned out=0; out<7; ++out) tout[out]=0.;
            tout[3]=-1.;
            /*Flush before fork: children inherit stdio buffers, and a child
             dying via exit(-1) in SP would otherwise flush a stale copy
             over the parent's file at the shared offset.*/
            _ds_out<<flush;
            _diag_step_out<<flush;
            _diag_var_out<<flush;
            _diag_move_out<<flush;
            cout<<flush;
            cerr<<flush;
            fflush(NULL);
            pid_t pid=fork();
            if (pid < 0) {
                BSP_ERROR<<"fork failed for dataset trial"<<endl;
                exit(-1);
            }
            if (pid==0) {
                /*child: fix v to dir, oracle-check, reconverge, report. Never returns.*/
                close(STDOUT_FILENO); close(STDERR_FILENO);
                _in_trial=true;
                _list_fixed_element.push_back(v);
                v->_it_list_fixed_elem=--_list_fixed_element.end();
                v->_I_am_a_fixed_variable=true;
                v->_who_I_am=(dir==1);
                if (_last_certitude>v->_sC) _last_certitude=v->_sC;
                clean(v);
                v->_degree_i=0;
                if (g_oracle && !_oracle_disabled) {
                    /*exact SAT label on the trial residual, before SP*/
                    ostringstream tmp;
                    tmp<<g_dataset_prefix<<"_or_"<<(int)getpid()<<".cnf";
                    _M_t=static_cast<unsigned>(_cl_list.size());
                    print_residual_to(tmp.str());
                    int sat=minisat_check(tmp.str());
                    if (sat==-4) _exit(126);/*minisat missing*/
                    if (sat==-3) _exit(43);/*minisat crashed: unknown, skip SP*/
                    tout[3]=sat;/*1, 0 or -2 (timeout)*/
                }
                stable_partition(ptrV.begin(), ptrV.end(), _Vertex_is_fixed_pred());
                if (g_lyapunov) {
                    g_epsilon=ZERO;
                    convergence_messages(8*t_max);
                } else {
                    convergence_messages();/*fatal exit -> converged=0*/
                }
                surveys();/*side effects are child-local*/
                tout[0]=1; tout[1]=complexity-sigma_before;
                tout[2]=(double)_time_conv_print;
                if (g_lyapunov) {
                    compute_lyapunov_at_fixed_point();
                    tout[4]=_lyapunov_rho;
                    tout[5]=_lyapunov_exponent;
                    tout[6]=complexity;
                }
                _exit(0);
            }
            int status=0;
            while (waitpid(pid, &status, 0) < 0 && errno==EINTR) { /*retry*/ }
            if (WIFEXITED(status) && WEXITSTATUS(status)==126) {
                BSP_ERROR<<"minisat not found at '"<<g_minisat_path
                         <<"'; use --minisat=PATH or --oracle=off"<<endl;
                exit(-1);
            }
            if (g_oracle) {
                ostringstream tmp;
                tmp<<g_dataset_prefix<<"_or_"<<(int)pid<<".cnf";
                unlink(tmp.str().c_str());
            }
            int converged=(WIFEXITED(status) && WEXITSTATUS(status)==0) ? 1 : 0;
            int crashed=(!converged && WIFSIGNALED(status)) ? 1 : 0;
            note_oracle_result((int)tout[3]);
            _ds_out<<step<<","<<mv<<","<<v->_vertex<<","<<dir<<","<<tail.str()<<","
                    <<converged<<","<<crashed<<","<<(int)tout[3]<<","
                    <<setprecision(10)<<(converged?tout[1]:0.)<<","
                    <<(converged?(int)tout[2]:-1)<<","
                    <<(converged?tout[4]:0.)<<","
                    <<(converged?tout[5]:0.)<<","
                    <<(converged?tout[6]:0.)<<"\n";
        }
    }
    _ds_out<<flush;
    munmap(tout, 7*sizeof(double));
#endif
}

/*Fill the 15-d feature vector used by bsp-train / GenANN inference.
 Layout matches tools that parse PREFIX_dataset.csv: surveys, bias, products,
 degree, incident unsat count, mean remaining clause length, Sigma/N, step
 fraction, r, and the assignment direction that fix_var_i() would pick.*/
void Graph::fill_nn_features(Vertex* v, double* f) {
    double sT=v->_sT, sF=v->_sF, sI=v->_sI;
    double abspol=fabs(sT-sF);
    double den=sT+sF;
    unsigned n_inc=0, len_sum=0;
    for (unsigned j=0; j<v->_I_am_in_cl_at_init.size(); ++j) {
        unsigned c=*real(v->_I_am_in_cl_at_init[j]);
        if (!_cl[c]._I_am_in_list_unsat) continue;
        ++n_inc;
        len_sum += (unsigned)_cl[c].size();
    }
    f[0]=sT;
    f[1]=sF;
    f[2]=sI;
    f[3]=1.0-((sT<sF)?sT:sF);
    f[4]=abspol;
    f[5]=(den>0.0)? abspol/den : 0.0;
    f[6]=v->prod_V_plus;
    f[7]=v->prod_V_minus;
    f[8]=static_cast<double>(v->_degree_i);
    f[9]=static_cast<double>(n_inc);
    f[10]=(n_inc>0)? (static_cast<double>(len_sum)/n_inc) : 0.0;
    f[11]=complexity/static_cast<double>(_N);
    f[12]=static_cast<double>(_N-_N_t)/static_cast<double>(_N);
    f[13]=g_r_bsp;
    f[14]=(sT>sF)? 1.0 : 0.0;
}

/*Overwrite unfixed _sC with predicted future complexity when a weights file
 is loaded (rank mode). In veto mode (--nn-veto=C) only bury vars scoring
 below C, keeping the hand-crafted bias otherwise. Out-of-distribution
 inputs always keep the hand-crafted score. The raw prediction (or NaN on
 abstention) is kept in _sNN for logging.*/
void Graph::apply_nn_scores() {
    if (!bsp_nn_ready()) return;
    unsigned n=0, kept=0, buried=0;
    for (unsigned i=0; i<_N; ++i) {
        Vertex* v=ptrV[i];
        if (v->_I_am_a_fixed_variable) continue;
        double x[BSP_NN_NFEAT];
        fill_nn_features(v, x);
        double y=0.0/0.0;
        ++n;
        if (!bsp_nn_predict(x, y)) { v->_sNN=y; continue; }
        v->_sNN=y;
        if (g_nn_veto) {
            if (y < g_nn_cutoff) { v->_sC=-1e100; ++buried; }
        } else {
            v->_sC=y;
            ++kept;
        }
    }
    if (g_nn_veto)
        BSP_INFO<<"NN veto buried "<<buried<<"/"<<n<<" unfixed variables"<<endl;
    else
        BSP_INFO<<"NN scorer applied to "<<kept<<"/"<<n<<" unfixed variables"<<endl;
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
void Graph::convergence_messages(unsigned max_iterations) { /*compute convergence messages for message passing algorithm*/
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
    for (unsigned int t=0; t<max_iterations; ++t) {
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
                            _new*=__pu();/*new message from cl to variable is computed*/
                            norm*=__norm();
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
                *_cl[C].v_survey_cl_to_i[i]=_cl[C].update[i];
                _cl[C].div_s[i]=Div_s(_cl[C].update[i]);
                ++i;
                if(i==s)break;
            }
        }
        update_products();/*update products into vertex node for speeding up the algorithm*/
        if(_counter_conv==0) { /*if counter convergence is zero, a convergence is found*/
            _M_t=static_cast<unsigned int>(_cl_list.size());
            // cout<<"I found a convergence at "<<t<<" "<<_m<<endl;
            _time_conv_print=t;
            update_complexity_clauses();/*update complexity_clause*/
            conv_f=true;
            break;
        }
    }
    if(!conv_f) { /*if after t_max iterations no convergence is found, the algorithm return exit failure */
        BSP_ERROR<<"SP does not find any fixed points -> SP does not converge."<<endl;
        BSP_ERROR<<"I am sorry I quit :("<<endl;
        exit(-1);
    }
}

/*public member class Graph. This member  updates only clause complexity.*/
void Graph::update_complexity_clauses() {
    double _ps=1.,_pu=1.;/*products for clause complexity*/
    unsigned long j;
    unsigned int C;
    //cout<<"This is the value where I start: "<<_M-_m<<endl;
    for(unsigned int __k=_m; __k<_M; ++__k) {
        C=(vec_list_cl[__k])->_c;
        _ps=1.;/*initialize to one product __ps*/
        _pu=1.;/*initialize to one product __pu*/
        /*updating complexity clauses*/
        for (j=0; j<_cl[C]._size_cl_init; ++j) { /*for each literal in a clause clauses complexity is updated */
            if(_cl[C]._go_forward[j]==1) {
                _prod_S=_Pr_S(_cl[C].v_V[j],_cl[C].v_lit[j], _cl[C].div_s[j]);
                _prod_U=_Pr_U(_cl[C].v_V[j],_cl[C].v_lit[j]);
                _ps*=__norm();/*update product for clause complexity*/
                _pu*=__pu();/*update product for clause complexity*/
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

/*public member class Graph. This memeber sorts the element of the list in ascending order, using as predicate the certitude, The first x components of the vector are not sorted because they contain fixed variables*/
void Graph::sort_V_Back_move() {
    sort(ptrV.begin(), ptrV.begin()+(_list_fixed_element.size()), _Vertex_greater_pred());
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
    if (g_dynamic_I_backtrack) {
        unsigned long count=_m_t_m_1+_unit_prop;
        _unit_prop=0;
        vector<pair<double, Vertex*> > candidates;
        candidates.reserve(_list_fixed_element.size());
        for (list<Vertex*>::iterator it=_list_fixed_element.begin();
             it!=_list_fixed_element.end(); ++it) {
            if (!(*it)->_forced_by_up)
                candidates.push_back(make_pair(current_fixation_factor(*it), *it));
        }
        sort(candidates.begin(), candidates.end());
        if (count>candidates.size()) count=candidates.size();
        for (unsigned long i=0; i<count; ++i) {
            Vertex* v=candidates[i].second;
            BSP_DEBUG<<"dynamic-I release v"<<v->_vertex
                     <<" I="<<candidates[i].first<<endl;
            _list_fixed_element.erase(v->_it_list_fixed_elem);
            v->_it_list_fixed_elem=_list_fixed_element.end();
            diag_move("back", v, -1);
            v->reset_value_default_var_i();
            build(v);
        }
        stable_partition(ptrV.begin(), ptrV.end(), _Vertex_is_fixed_pred());
        _N_t=_N-static_cast<unsigned>(_list_fixed_element.size());
        return;
    }
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


