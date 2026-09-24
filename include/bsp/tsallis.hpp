//
//  tsallis.hpp
//  BSP
//
//  Tsallis deformation of the cluster weights on the 1/N scale (Cor. yeff of
//  paper/sections/qaxis.tex).
//

#ifndef TSALLIS_HPP
#define TSALLIS_HPP

/*kappa = N(q-1) of the Tsallis weight e_q(-yE). kappa=0 is plain SP-y.
 kappa<0 (q<1) gives a convex g_kappa and an energy cut e < 1/(|kappa| y).
 Set with --tsallis-kappa=K, needs --cav-temp>0.*/
extern double g_tsallis_kappa;

/*Base inverse temperature y = 1/T_cav given on the command line. The SP-y
 equations run at y_eff, stored as g_T_cav = 1/y_eff.*/
extern double g_tsallis_y;

/*T_eff = 1/y_eff = (1 + kappa y e) / y. Returns 0 (y_eff infinite) when
 1 + kappa y e <= 0: the energy is beyond the cut of kappa<0, so the measure
 keeps only e=0. The inverse form avoids infinities under -ffast-math.*/
double tsallis_T_eff(double y, double kappa, double e);

/*One step of the outer loop: e = E/N_t from the Bethe energy of the last
 fixed point, then g_T_cav = 1/y_eff (0 when y_eff is infinite). At T_cav=0
 the energy is 0, so the next step returns to y_eff = y.*/
void tsallis_update(double E, unsigned N_t);

#endif /* TSALLIS_HPP */
