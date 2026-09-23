//
//  thermo_sp.hpp
//  BSP
//
//  Thermodynamic Survey Propagation: deformed cavity star sums.
//  See thermodynamic-sp.md for the derivation and the T=0 limit.
//

#ifndef THERMO_SP_HPP
#define THERMO_SP_HPP

#include <cstddef>

/*Cavity temperature of the thermodynamic deformation. T=0 keeps the tropical
 (hard) SP factors of Graph::__pu and Graph::__norm. Set with --cav-temp=T.*/
extern double g_T_cav;

/*Cluster reweighting exponent of the 1RSB measure mu_m(C) proportional to
 e^(m N s_C). m=0 keeps the uniform cluster measure. SP messages do not carry
 the internal entropy s_C of a cluster, so the code uses the Maneva-Mossel-
 Wainwright closure s_C ~ ln2 q_C, with q_C the fraction of free variables.
 Under this closure m acts as the unfrozen tilt gamma = m ln2 (see
 g_gamma_eff). Set with --rsb-m=M.*/
extern double g_rsb_m;

/*Unfrozen-cluster bias of mu_{m,gamma}(C) proportional to exp[N(m s_C+gamma q_C)],
 where q_C is the fraction of variables with no warning. gamma multiplies only
 the hard unfrozen sector A0*B0 of a cavity star by e^gamma; frozen sectors and
 the finite-T balanced conflicts (p=q>0) stay put. gamma=0 is ordinary SP.
 This is not --scorer=gamma:<g>, which only ranks decimation. Set with
 --rsb-gamma=G. With gamma!=0 the printed Sigma is the free entropy of this
 biased measure, not the uniform complexity at m=0, gamma=0.*/
extern double g_rsb_gamma;

/*Unfrozen tilt that the star sums use: g_rsb_gamma + g_rsb_m ln2. main.cpp
 sets it once after parsing the options.*/
extern double g_gamma_eff;

/*Deformed Pi sums of one cavity star. eta_s holds the messages that push the
 cavity variable to satisfy the target clause, eta_u the messages that push it
 the other way. A warning configuration omega draws each message as active with
 probability eta, p and q count the active warnings of the two groups, and
 min(p,q) is its conflict energy. The Gibbs weight e^(-min(p,q)/T) deforms the
 hard aggregation of the note: pi_u collects the configurations with q>p, pi_s
 those with p>q, pi_0 those with p=q, z is the partition sum, and e_mean is the
 mean conflict energy under the Gibbs weight.
 A0 and B0 are the products of (1-eta) over the two groups with the cavity
 message already excluded, in the exact form used by the legacy factors (give
 the values from _Pr_S and _Pr_U, or from prod_V_plus and prod_V_minus). The
 conflict-free configurations then give the hard factors (1-B_0)*A_0,
 (1-A_0)*B_0, A_0*B_0 and A_0+B_0-A_0*B_0 exactly, and the sums over the
 conflicting configurations add the finite-T corrections on top. At T=0 and
 gamma=0 the corrections vanish and the sums equal __pu and __norm. gamma
 replaces the hard unfrozen mass A_0*B_0 by e^gamma*A_0*B_0 and rebuilds z;
 the p=q>0 corrections are not multiplied by e^gamma.*/
struct ThermoStar {
  double pi_u;
  double pi_s;
  double pi_0;
  double z;
  double e_mean;
};

ThermoStar thermo_star(const double *eta_s, std::size_t ns, const double *eta_u,
                       std::size_t nu, double A0, double B0, double T,
                       double gamma = 0.);

#endif /* THERMO_SP_HPP */
