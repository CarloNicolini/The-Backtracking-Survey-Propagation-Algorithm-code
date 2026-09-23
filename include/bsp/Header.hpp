//
//  Header.h
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
//  Created by Raffaele Marino on 17/10/2018.
//  Copyright © 2018 Raffaele Marino. All rights reserved.
//

#ifndef BSP_Header_hpp
#define BSP_Header_hpp
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <vector>
#include <iterator>
#include <algorithm>
#include <valarray>
#include <numeric>
#include <complex>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <iomanip>
#include <list>
#include <sstream>
#include <bsp/random.hpp>
#include <bsp/Logger.hpp>

#define WALKSAT
#define BP
#define ZERO (1.0E-10)/*all values less than 1.0e-10 are zero*/
#define PRINT_FORMULA_CNF
//#define POL
#define CERT
//#define P_M
//#define I_C


#define INITLEN 32
/*if SAT is un-commented will be a sat-solver*/
#define SAT
/*if XORSAT is un-commented will be a xorsat-solver*/
//#define XORSAT
/*if NAESAT is un-commented will be a naesat-solver*/
//#define NAESAT



using namespace std;

const string directory="./";/*directory for output files*/
enum{t_max=1024}; /*maximum number of iteration for a message passing algorithm*/
const double rho_SP=1.;/*from SP to BP when it is set to 0*/
const double frac=0.00125; /*fraction of variables for decimation*/
const double e=2.7182818284590452353602874713527;/*constant e*/
const double pi=3.141592653589793238462643383279;/*constant p*/
const double epsilon=0.01;/*convergence epsilon value*/
const double _R_BSP=0.9;/*r values for BSP, it goes from [0, 1).  When it is equal to 0, one gets a SID algorithms, while when it is different to 0 one gets the BSP. */

#ifdef WALKSAT
int WalkSat(vector <vector<bool> > & sol,int argc, char * argv[]); // WalkSat function
#endif

/*Runtime decimation-scorer selection. -1 keeps the compiled-in __H macro
 (legacy behavior, CERT by default); 0=CERT, 1=POL, 2=GAMMA, 3=I_C, 4=FTH,
 5=RSB. GAMMA scores b*|sT-sF|^g_scorer_gamma, interpolating certainty (g=0)
 and polarization-like rankings. FTH ranks by the absolute free-energy bias and
 needs --cav-temp>0. RSB is b_i(m), the bias of the 1RSB measure. Set via
 --scorer=... (see main.cpp).*/
extern int g_scorer_id;
extern double g_scorer_gamma;
double bsp_score(double a, double b, double c, double b_th); /*a=sT,b=sF,c=sI,b_th=Phi_minus-Phi_plus*/
bool bsp_parse_scorer(const string& spec); /*"cert","pol","i_c","gamma:<g>"*/
string bsp_scorer_name(); /*short name of the active scorer, for logging*/

/*Per-step diagnostic logging (Phase 1). Off unless --diag=PREFIX is given.*/
extern string g_diag_prefix; /*output prefix for _steps.csv / _vars.csv */
extern unsigned g_diag_every; /*log vars + residuals every K steps, default 1*/
extern bool g_dump_residuals; /*also dump PREFIX_res_s<step>.cnf files*/

/*Runtime overrides for the BSP backtracking ratio r and the RNG seed.
 Defaults reproduce legacy behavior (_R_BSP and /dev/urandom).*/
extern double g_r_bsp; /*backtracking ratio, must stay in [0, 1)*/
extern long g_fixed_seed; /*>=1 fixes the RNG seed, -1 keeps /dev/urandom*/

/*Phase 2 decimation controls. Defaults reproduce legacy behavior.*/
extern double g_bsp_theta; /*min direction margin |sT-sF|/(sT+sF) to fix, default 0 (off)*/
extern bool g_veto; /*veto co-decimating vars sharing a clause, default off*/
extern double g_epsilon; /*SP convergence threshold, default epsilon*/
extern double g_damping; /*SP update damping in [0,1), default 0 (off)*/
bool bsp_pass_margin(double sT, double sF); /*true if margin >= g_bsp_theta*/

/*Minisat oracle for --oracle-dir (exact 1-step direction check). Off by default.*/
extern string g_minisat_path; /*minisat binary, default "minisat"*/
extern unsigned g_oracle_timeout; /*per-check minisat seconds, default 10 (0 = unbounded)*/
extern bool g_oracle_dir; /*check both dirs per chosen var, take a SAT one*/
extern unsigned g_oracle_pick; /*scan top-K for SAT-preserving (var,dir), 0 = off*/
extern unsigned g_lookahead_k; /*top-K complexity lookahead, 0 = off*/
extern bool g_dynamic_i; /*release by I(k) from current surveys, default off*/
extern bool g_fe_backtrack; /*free-energy release order and Gibbs move split, default off*/
extern double g_bt_cost; /*cost of one back move in the Gibbs split, default 0.4*/
/*Complexity-profile controls. All off by default, so legacy BSP is unchanged.
 Lookahead and corr-batch pick a move by the realized Sigma after a forked
 SP reconvergence. Adaptive-r raises the backtracking ratio when Sigma falls
 steeply or SP converges slowly. g_frac overrides the compiled-in batch fraction.*/
extern bool g_corr_batch; /*among high-P vars, prefer a distance-2 batch*/
extern bool g_adaptive_r; /*raise r when the Sigma slope is steep or SP is slow*/
extern double g_frac; /*decimation batch fraction, default frac*/

#endif /* BSP_Header_hpp */






