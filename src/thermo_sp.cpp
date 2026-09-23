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
double g_gamma_eff = 0.;

void thermo_coefficients(const double *eta, size_t n, double *coef) {
  coef[0] = 1.;
  for (size_t b = 0; b < n; ++b) {
    coef[b + 1] = coef[b] * eta[b]; /*the top coefficient starts at zero*/
    for (size_t p = b; p > 0; --p) {
      coef[p] = coef[p] * (1. - eta[b]) + coef[p - 1] * eta[b];
    }
    coef[0] *= (1. - eta[b]);
  }
}

/*Gibbs weight e^(-k/T) of k conflicts, cached while T stays the same.*/
struct BoltzmannTable {
  double T = -1.;
  vector<double> w;
};

ThermoStar thermo_star_coef(const double *A, size_t ns, const double *B,
                            size_t nu, double A0, double B0, double T,
                            double gamma) {
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
  if (ns == 0 || nu == 0) return out; /*a conflict needs warnings on both sides*/
  static thread_local BoltzmannTable table;
  BoltzmannTable &bt = table;
  size_t kmax = (ns < nu) ? ns : nu;
  if (bt.T != T) {
    bt.w.clear();
    bt.T = T;
  }
  for (size_t k = bt.w.size(); k <= kmax; ++k)
    bt.w.push_back(exp(-(double)k / T));
  const double *boltz = bt.w.data();
  /*The weight of (p,q) is A_p B_q e^(-min(p,q)/T). In the sector q>p it
   depends on p only, so the sector is sum_p A_p e^(-p/T) sum_{q>p} B_q. The
   tails are summed from the top, as sums of positive terms.*/
  double cu = 0., cs = 0., c0 = 0., ew = 0.;
  double tail_b = 0., tail_a = 0.;
  for (size_t q = nu; q > kmax; --q)
    tail_b += B[q];
  for (size_t p = ns; p > kmax; --p)
    tail_a += A[p];
  for (size_t k = kmax; k >= 1; --k) {
    double wa = A[k] * boltz[k];
    double wb = B[k] * boltz[k];
    double wu = wa * tail_b; /*p=k, q>k*/
    double ws = wb * tail_a; /*q=k, p>k*/
    double w0 = wa * B[k];   /*p=q=k*/
    cu += wu;
    cs += ws;
    c0 += w0;
    ew += (double)k * (wu + ws + w0);
    tail_b += B[k];
    tail_a += A[k];
  }
  out.pi_u += cu;
  out.pi_s += cs;
  out.pi_0 += c0;
  out.z += cu + cs + c0;
  out.e_mean = (out.z > 0.) ? ew / out.z : 0.;
  return out;
}

ThermoStar thermo_star(const double *eta_s, size_t ns, const double *eta_u,
                       size_t nu, double A0, double B0, double T, double gamma) {
  if (T <= 0. || ns == 0 || nu == 0)
    return thermo_star_coef(NULL, 0, NULL, 0, A0, B0, T, gamma);
  vector<double> A(ns + 1), B(nu + 1);
  thermo_coefficients(eta_s, ns, A.data());
  thermo_coefficients(eta_u, nu, B.data());
  return thermo_star_coef(A.data(), ns, B.data(), nu, A0, B0, T, gamma);
}
