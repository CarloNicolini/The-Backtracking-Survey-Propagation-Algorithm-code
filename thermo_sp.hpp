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

/*Deformed Pi sums of one cavity star. eta_s holds the messages that push the
 cavity variable to satisfy the target clause, eta_u the messages that push it
 the other way. A warning configuration omega draws each message as active with
 probability eta, p and q count the active warnings of the two groups, and
 min(p,q) is its conflict energy. The Gibbs weight e^(-min(p,q)/T) deforms the
 hard aggregation of the note: pi_u collects the configurations with q>p, pi_s
 those with p>q, pi_0 those with p=q, and z is the partition sum. At T=0 only
 the conflict-free configurations keep weight 1 and the sums return the hard SP
 factors (1-B_0)*A_0, (1-A_0)*B_0, A_0*B_0 with A_0 and B_0 the products of
 (1-eta) over the two groups.*/
struct ThermoStar {
    double pi_u;
    double pi_s;
    double pi_0;
    double z;
};

ThermoStar thermo_star(const double *eta_s, std::size_t ns,
                       const double *eta_u, std::size_t nu,
                       double T);

#endif /* THERMO_SP_HPP */
