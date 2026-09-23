//
//  Vertex.cpp
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

/*In this file public, non-inline, members for class Vertex are defined.*/

#include <bsp/Vertex.hpp>

/*Runtime decimation-scorer selection (see Header.h). Default -1 reproduces
 the legacy compiled-in __H macro behavior exactly.*/
int g_scorer_id = -1;
double g_scorer_gamma = 1.0;

/*Score a variable for (de)selection from its surveys: a=sT, b=sF, c=sI,
 b_th=free-energy bias Phi_minus-Phi_plus.*/
double bsp_score(double a, double b, double c, double b_th) {
    double bias = 1.0 - std::min(a, b); /*CERT*/
    double pol = std::fabs(a - b); /*POL*/
    switch (g_scorer_id) {
        case 0: return bias;
        case 1: return pol;
        case 2: return bias * std::pow(pol, g_scorer_gamma);
        case 3: return c; /*I_C*/
        case 4: return std::fabs(b_th); /*FTH, the free-energy bias*/
        case 5: return std::fabs(a - b); /*RSB, b_i(m) of the 1RSB measure*/
        default: return __H(a, b, c); /*legacy compiled-in macro*/
    }
}

/*Parse a --scorer=... spec: "cert", "pol", "i_c", "fth", "rsb" or "gamma:<g>", g>=0.*/
bool bsp_parse_scorer(const string& spec) {
    if (spec == "cert") { g_scorer_id = 0; return true; }
    if (spec == "pol") { g_scorer_id = 1; return true; }
    if (spec == "i_c") { g_scorer_id = 3; return true; }
    if (spec == "fth") { g_scorer_id = 4; return true; }
    if (spec == "rsb") { g_scorer_id = 5; return true; }
    if (spec.compare(0, 6, "gamma:") == 0) {
        try {
            g_scorer_gamma = std::stod(spec.substr(6));
        } catch (...) {
            return false;
        }
        if (!(g_scorer_gamma >= 0.0)) return false;
        g_scorer_id = 2;
        return true;
    }
    return false;
}

/*Short name of the active scorer, for logging.*/
string bsp_scorer_name() {
    switch (g_scorer_id) {
        case 0: return "cert";
        case 1: return "pol";
        case 3: return "i_c";
        case 4: return "fth";
        case 5: return "rsb";
        case 2: {
            ostringstream o;
            o << "gamma:" << g_scorer_gamma;
            return o.str();
        }
        default: return "macro-default";
    }
}

/*Direction-margin gate (Phase 2): fix only if |sT-sF|/(sT+sF) >= g_bsp_theta.
 Fully unconstrained vars (sT+sF==0) have margin 0. theta=0 passes all. */
bool bsp_pass_margin(double sT, double sF) {
    double denom = sT + sF;
    double margin = (denom > 0.0) ? (fabs(sT - sF) / denom) : 0.0;
    return margin >= g_bsp_theta;
}

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***************************** START VERTEX CLASS **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


/*public member which computes surveys sT, sF, sI and certitude for each variable node.
 Moreover, the function computes the own associated variable complexity.*/
void Vertex::compute_s() { /*compute surveys variable node*/
    double _p_plus=0.,_p_minus=0.,_p_I=0.;/*declaration local variables*/
    complexity_variable=0.;/*set variable complexity to zero*/
    if(g_T_cav>0. || g_rsb_gamma!=0.) {/*ThermoSP or unfrozen bias: field star*/
        ThermoStar _st=cavity_star(true,NULL,prod_V_plus,prod_V_minus,g_T_cav,g_rsb_gamma);
        _p_plus=_st.pi_s;/*p>q region*/
        _p_minus=_st.pi_u;/*q>p region*/
        _p_I=_st.pi_0;
        if(g_T_cav>0.) {/*free energies need a cavity temperature*/
            /*1e-300 floors the log of an impossible direction, so an
             unconstrained variable keeps bias 0*/
            double _fp=(_p_plus>1e-300)?_p_plus:1e-300;
            double _fm=(_p_minus>1e-300)?_p_minus:1e-300;
            _phi_plus=-g_T_cav*log(_fp);
            _phi_minus=-g_T_cav*log(_fm);
            _B_th=_phi_minus-_phi_plus;
        } else {
            _phi_plus=0.;
            _phi_minus=0.;
            _B_th=0.;
        }
    } else {
        _p_plus=_p_PLUS();/*compute bias _pi_plus*/
        _p_minus=_p_MINUS();/*compute bias _pi_minus*/
        _p_I=_p_IND();/*compute bias _pi_indeterminate*/
    }
    complexity_variable=_p_I+_p_minus+_p_plus;/*update variable node complexity*/
    _sT=S(_p_plus, _p_minus, _p_I);/*compute survey _sT variable node*/
    _sF=S(_p_minus, _p_plus, _p_I);/*compute survey _sF variable node*/
    _sI=1.-_sT-_sF;/*compute survey _sI variable node*/
    _sC=S_C();/*compute certitude variable node i*/
}

/*public member which updates products (1-survey) for V_plus and V_minus sets. These products are usful for computing SP and BP equations in an easy way.*/
void Vertex::make_products() { /*make products for set V_plus and V_minus*/
    bool _snap=(g_T_cav>0. or g_rsb_m!=0. or g_rsb_gamma!=0.);/*cache values for the star sums*/

    prod_V_plus=1.;/*set the product of V_plus to 1*/
    if(_snap)snap_plus.resize(_surveys_cl_to_i_plus.size());/*cache values for ThermoSP stars*/
    for (unsigned long i=0; i<_surveys_cl_to_i_plus.size(); ++i) {/*loop for updating vector, where are stored  messages, and products (1-survey) for V_plus and V_minus sets.*/
        if(_snap)snap_plus[i]=*_surveys_cl_to_i_plus[i];
        prod_V_plus*=(1.- *_surveys_cl_to_i_plus[i]);/*compute products for V_plus*/
    }
    prod_V_minus=1.;/*set the product of V_minus to 1*/
    if(_snap)snap_minus.resize(_surveys_cl_to_i_minus.size());/*cache values for ThermoSP stars*/
    for (unsigned long i=0; i<_surveys_cl_to_i_minus.size(); ++i) {/*loop for updating vector, where are stored  messages, and products (1-survey) for V_plus and V_minus sets.*/
        if(_snap)snap_minus[i]=*_surveys_cl_to_i_minus[i];
        prod_V_minus*=(1.- *_surveys_cl_to_i_minus[i]);/*compute products for V_minus*/
    }
}

/*public member which computes the deformed cavity star sums at this variable.
 The message values come from the last make_products call, so the star sees
 exactly the values behind the legacy products. The message of the clause that
 owns the cavity is passed as exclude and skipped in the groups (it always
 belongs to the s group, the one selected by b, like the div_s correction of the
 legacy factors). A0 and B0 are the hard factors of the two groups with the
 cavity message already excluded: pass the exact values of _Pr_S and _Pr_U, or
 prod_V_plus and prod_V_minus for the field star.*/
ThermoStar Vertex::cavity_star(bool b, const double *exclude, double A0, double B0, double T, double gamma) {
    const vector<double *> &_s_ptr=(b)?_surveys_cl_to_i_plus:_surveys_cl_to_i_minus;
    const vector<double *> &_u_ptr=(b)?_surveys_cl_to_i_minus:_surveys_cl_to_i_plus;
    const vector<double> &_s_val=(b)?snap_plus:snap_minus;
    const vector<double> &_u_val=(b)?snap_minus:snap_plus;
    vector<double> _es,_eu;
    _es.reserve(_s_ptr.size());
    _eu.reserve(_u_ptr.size());
    for (unsigned long i=0; i<_s_ptr.size(); ++i)
        if(_s_ptr[i]!=exclude)_es.push_back(_s_val[i]);
    for (unsigned long i=0; i<_u_ptr.size(); ++i)
        if(_u_ptr[i]!=exclude)_eu.push_back(_u_val[i]);
    return thermo_star(_es.data(),_es.size(),_eu.data(),_eu.size(),A0,B0,T,gamma);
}


/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/******************************  END VERTEX CLASS  **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
