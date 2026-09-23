//
//  thermo_sp.cpp
//  BSP
//
//  Thermodynamic Survey Propagation: deformed cavity star sums.
//

#include <bsp/thermo_sp.hpp>

#include <bsp/Header.hpp>

#include <cmath>
#include <vector>

using namespace std;

double g_T_cav = 0.;
double g_rsb_m = 0.;
double g_rsb_gamma = 0.;

double thermo_tilt(double eta, double kappa_warn, double kappa_sil, double m) {
  if (m == 0.) return eta;
  /*a branch with no cluster count has no weight in mu_m for any m*/
  double w = (kappa_warn > 0.) ? eta * pow(kappa_warn, m) : 0.;
  double s = (kappa_sil > 0.) ? (1. - eta) * pow(kappa_sil, m) : 0.;
  double t = w + s;
  return (t > 0.) ? w / t : eta;
}

/*Coefficients of the polynomial product over one warning group:
 prod_b (1 - eta_b + eta_b z) = sum_p A_p z^p. coef must hold n+1 values and
 A_p is the prior probability of p active warnings in the group. The callers
 pass their own A_0 and B_0 values, so only the coefficients with p >= 1 are
 used.*/
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
                       size_t nu, double A0, double B0, double T, double gamma) {
  ThermoStar out = {0., 0., 0., 0., 0.};
  out.pi_u = (1. - B0) * A0;
  out.pi_s = (1. - A0) * B0;
  /*Hard unfrozen mass only. Balanced conflicts added below stay unboosted.*/
  double pi0_hard = A0 * B0;
  double w_free = (gamma == 0.) ? 1. : exp(gamma);
  out.pi_0 = pi0_hard * w_free;
  if (gamma == 0.)
    out.z = A0 + B0 - (rho_SP * A0 * B0); /*legacy association at gamma=0*/
  else
    out.z = out.pi_u + out.pi_s + out.pi_0;
  if (T <= 0.) return out; /*tropical limit: the corrections vanish*/
  vector<double> A(ns + 1), B(nu + 1);
  group_coefficients(eta_s, ns, &A[0]);
  group_coefficients(eta_u, nu, &B[0]);
  double cz = 0., ew = 0.;
  for (size_t p = 1; p <= ns; ++p) {
    for (size_t q = 1; q <= nu; ++q) {
      double conflict = (p < q) ? (double)p : (double)q;
      double w = A[p] * B[q] * exp(-conflict / T);
      cz += w;
      ew += conflict * w;
      if (q > p)
        out.pi_u += w;
      else if (p > q)
        out.pi_s += w;
      else
        out.pi_0 += w;
    }
  }
  out.z += cz;
  out.e_mean = (out.z > 0.) ? ew / out.z : 0.;
  return out;
}
