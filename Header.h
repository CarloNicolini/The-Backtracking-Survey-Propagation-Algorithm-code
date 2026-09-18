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

#ifndef Header_h
#define Header_h
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
#include "random.h"
#include "Logger.hpp"

//#define NeurNET
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
 (legacy behavior, CERT by default); 0=CERT, 1=POL, 2=GAMMA, 3=I_C.
 GAMMA scores b*|sT-sF|^g_scorer_gamma, interpolating certainty (g=0)
 and polarization-like rankings. Set via --scorer=... (see main.cpp).*/
extern int g_scorer_id;
extern double g_scorer_gamma;
double bsp_score(double a, double b, double c); /*a=sT,b=sF,c=sI*/
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
extern bool g_dynamic_I_backtrack; /*rank releases by current assignment retention*/
extern bool g_flexibility_diag; /*log SP indeterminacy/flexibility observables*/
bool bsp_pass_margin(double sT, double sF); /*true if margin >= g_bsp_theta*/

/*Phase 3 DeltaSigma dataset (POSIX only, uses fork). Off unless --dataset=PREFIX.*/
extern string g_dataset_prefix; /*output prefix for _dataset.csv*/
extern unsigned g_dataset_k; /*shortlist size per step, default 50*/
extern unsigned g_dataset_every; /*trial cadence in SP steps, default 1*/
extern bool g_oracle; /*exact SAT-oracle label per trial via minisat, default off*/
extern string g_minisat_path; /*minisat binary, default "minisat"*/
extern unsigned g_oracle_timeout; /*per-trial minisat seconds, default 10 (0 = unbounded)*/
/*Oracle-guided decimation (exact 1-step lookahead, POSIX). Both off by default.*/
extern bool g_oracle_dir; /*check both dirs per chosen var, take a SAT one*/
extern unsigned g_oracle_pick; /*scan top-K for SAT-preserving (var,dir), 0 = off*/
extern unsigned g_lookahead_k; /*top-K complexity lookahead, 0 = off*/
extern string g_nn_path; /*GenANN weights from bsp-train; empty = off*/
extern bool g_nn_veto; /*veto mode: bury vars scoring below cutoff, else keep bias*/
extern double g_nn_cutoff; /*veto threshold on predicted DeltaSigma*/
const int BSP_NN_NFEAT = 15;

#endif /* Header_h */






