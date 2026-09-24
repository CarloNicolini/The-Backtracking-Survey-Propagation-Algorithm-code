//
//  tsallis.cpp
//  BSP
//
//  Outer loop of SP-y at the effective y of the Tsallis weight.
//

#include <bsp/tsallis.hpp>
#include <bsp/thermo_sp.hpp>

double g_tsallis_kappa = 0.;
double g_tsallis_y = 0.;

double tsallis_T_eff(double y, double kappa, double e) {
  double d = 1. + kappa * y * e;
  return (d > 0.) ? d / y : 0.;
}

void tsallis_update(double E, unsigned N_t) {
  double e = (N_t > 0) ? E / N_t : 0.;
  g_T_cav = tsallis_T_eff(g_tsallis_y, g_tsallis_kappa, e);
}
