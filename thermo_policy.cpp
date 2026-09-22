//
//  thermo_policy.cpp
//  BSP
//
//  Thermodynamic Survey Propagation: Gibbs decision policy (pi_T of the note).
//

#include "thermo_policy.hpp"

#include <cmath>
#include <cstdlib>

using namespace std;

double g_T_act = 0.;

bool thermo_gibbs_direction(double phi_plus, double phi_minus, double T_act) {
  if (T_act <= 0.) return phi_plus < phi_minus; /*hard rule, false on ties*/
  /*P(true) = e^(-phi+/T) / (e^(-phi+/T) + e^(-phi-/T)) = 1/(1+exp((phi+-phi-)/T))*/
  double p_true = 1. / (1. + exp((phi_plus - phi_minus) / T_act));
  double u = random() / (RAND_MAX + 1.0);
  return u < p_true;
}
