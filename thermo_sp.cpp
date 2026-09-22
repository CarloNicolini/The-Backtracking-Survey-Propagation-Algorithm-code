//
//  thermo_sp.cpp
//  BSP
//
//  Thermodynamic Survey Propagation: deformed cavity star sums.
//

#include "thermo_sp.hpp"

#include <cmath>
#include <vector>

using namespace std;

double g_T_cav = 0.;

/*Coefficients of the polynomial product over one warning group:
 prod_b (1 - eta_b + eta_b z) = sum_p A_p z^p. coef must hold n+1 values and
 A_p is the prior probability of p active warnings in the group.*/
static void group_coefficients(const double *eta, size_t n, double *coef) {
  for (size_t p = 0; p <= n; ++p)
    coef[p] = 0.;
  coef[0] = 1.;
  for (size_t b = 0; b < n; ++b) {
    for (size_t p = b + 1; p > 0; --p) {
      coef[p] = coef[p] * (1. - eta[b]) + coef[p - 1] * eta[b];
    }
    coef[0] *= (1. - eta[b]);
  }
}

ThermoStar thermo_star(const double *eta_s, size_t ns, const double *eta_u,
                       size_t nu, double T) {
  vector<double> A(ns + 1), B(nu + 1);
  group_coefficients(eta_s, ns, &A[0]);
  group_coefficients(eta_u, nu, &B[0]);
  ThermoStar out = {0., 0., 0., 0.};
  if (T <= 0.) { /*tropical limit: keep only min(p,q)=0*/
    out.pi_u = (1. - B[0]) * A[0];
    out.pi_s = (1. - A[0]) * B[0];
    out.pi_0 = A[0] * B[0];
  } else {
    for (size_t p = 0; p <= ns; ++p) {
      for (size_t q = 0; q <= nu; ++q) {
        double conflict = (p < q) ? (double)p : (double)q;
        double w = A[p] * B[q] * exp(-conflict / T);
        if (q > p)
          out.pi_u += w;
        else if (p > q)
          out.pi_s += w;
        else
          out.pi_0 += w;
      }
    }
  }
  out.z = out.pi_u + out.pi_s + out.pi_0;
  return out;
}
