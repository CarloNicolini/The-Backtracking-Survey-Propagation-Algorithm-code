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

#include "Vertex.hpp"

/*Runtime decimation-scorer selection (see Header.h). Default -1 reproduces
 the legacy compiled-in __H macro behavior exactly.*/
int g_scorer_id = -1;
double g_scorer_gamma = 1.0;

/*Score a variable for (de)selection from its surveys: a=sT, b=sF, c=sI.*/
double bsp_score(double a, double b, double c) {
    double bias = 1.0 - std::min(a, b); /*CERT*/
    double pol = std::fabs(a - b); /*POL*/
    switch (g_scorer_id) {
        case 0: return bias;
        case 1: return pol;
        case 2: return bias * std::pow(pol, g_scorer_gamma);
        case 3: return c; /*I_C*/
        default: return __H(a, b, c); /*legacy compiled-in macro*/
    }
}

/*Parse a --scorer=... spec: "cert", "pol", "i_c" or "gamma:<g>", g>=0.*/
bool bsp_parse_scorer(const string& spec) {
    if (spec == "cert") { g_scorer_id = 0; return true; }
    if (spec == "pol") { g_scorer_id = 1; return true; }
    if (spec == "i_c") { g_scorer_id = 3; return true; }
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

/*Lookahead trial energy (E4a). Default 0=sigma reproduces legacy lookahead.*/
int g_energy_id = 0;

bool bsp_parse_energy(const string& spec) {
    if (spec == "sigma") { g_energy_id = 0; return true; }
    if (spec == "eta") { g_energy_id = 1; return true; }
    if (spec == "hybrid") { g_energy_id = 2; return true; }
    return false;
}

string bsp_energy_name() {
    switch (g_energy_id) {
        case 1: return "eta";
        case 2: return "hybrid";
        default: return "sigma";
    }
}

/*E4b rollout controls. H<=1 (default 0) disables multi-step trials.*/
int g_rollout_h = 0;
unsigned g_rollout_eta = 15;

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
    _p_plus=_p_PLUS();/*compute bias _pi_plus*/
    _p_minus=_p_MINUS();/*compute bias _pi_minus*/
    _p_I=_p_IND();/*compute bias _pi_indeterminate*/
    complexity_variable=_p_I+_p_minus+_p_plus;/*update variable node complexity*/
    _sT=S(_p_plus, _p_minus, _p_I);/*compute survey _sT variable node*/
    _sF=S(_p_minus, _p_plus, _p_I);/*compute survey _sF variable node*/
    _sI=1.-_sT-_sF;/*compute survey _sI variable node*/
    _sC=S_C();/*compute certitude variable node i*/
}

/*public member which updates products (1-survey) for V_plus and V_minus sets. These products are usful for computing SP and BP equations in an easy way.*/
void Vertex::make_products() { /*make products for set V_plus and V_minus*/

    prod_V_plus=1.;/*set the product of V_plus to 1*/
    for (unsigned long i=0; i<_surveys_cl_to_i_plus.size(); ++i) {/*loop for updating vector, where are stored  messages, and products (1-survey) for V_plus and V_minus sets.*/
        prod_V_plus*=(1.- *_surveys_cl_to_i_plus[i]);/*compute products for V_plus*/
    }
    prod_V_minus=1.;/*set the product of V_minus to 1*/
    for (unsigned long i=0; i<_surveys_cl_to_i_minus.size(); ++i) {/*loop for updating vector, where are stored  messages, and products (1-survey) for V_plus and V_minus sets.*/
        prod_V_minus*=(1.- *_surveys_cl_to_i_minus[i]);/*compute products for V_minus*/
    }
}


/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/******************************  END VERTEX CLASS  **********************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
