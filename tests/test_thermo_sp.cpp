//
//  test_thermo_sp.cpp
//  BSP tests
//
//  Numeric identities of the thermodynamic star sums (thermodynamic-sp.md):
//  1. the sums match a brute-force enumeration of the warning configurations;
//  2. the T=0 and tiny-T sums equal the hard SP factors exactly;
//  3. the partition sum z equals pi_u+pi_s+pi_0;
//  4. the Gibbs sampler frequencies match the weights e^(-Phi/T_act).
//

#include <bsp/thermo_sp.hpp>
#include <bsp/thermo_policy.hpp>

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace std;

static int failures = 0;

static void expect_close(const char *what, double got, double want, double tol) {
    if (!(fabs(got - want) <= tol)) {
        fprintf(stderr, "FAIL %s: got %.17g want %.17g\n", what, got, want);
        ++failures;
    }
}

/*Reference sums from the definition: enumerate every warning configuration,
 weight it with its two state masses times e^(-min(p,q)/T), and sort it by
 region. The masses of one message are the weights of its silent and warning
 states, so the call passes eta and 1-eta for the plain prior.*/
static ThermoStar brute_star(const vector<double> &ja, const vector<double> &j0,
                             const vector<double> &ua, const vector<double> &u0,
                             double T) {
  ThermoStar out = {0., 0., 0., 0., 0.};
  const size_t ns = ja.size(), nu = ua.size();
  const size_t ms = 1u << ns, mu = 1u << nu;
  double ew = 0.;
  for (size_t xs = 0; xs < ms; ++xs) {
    for (size_t xu = 0; xu < mu; ++xu) {
      size_t p = 0, q = 0;
      double prob = 1.;
      for (size_t b = 0; b < ns; ++b) {
        if (xs & (1u << b)) {
          ++p;
          prob *= ja[b];
        } else
          prob *= j0[b];
      }
      for (size_t b = 0; b < nu; ++b) {
        if (xu & (1u << b)) {
          ++q;
          prob *= ua[b];
        } else
          prob *= u0[b];
      }
      double conflict = (p < q) ? (double)p : (double)q;
      double w = (T > 0.) ? prob * exp(-conflict / T)
                          : ((conflict == 0.) ? prob : 0.);
      ew += conflict * w;
      if (q > p)
        out.pi_u += w;
      else if (p > q)
        out.pi_s += w;
      else
        out.pi_0 += w;
    }
  }
  out.z = out.pi_u + out.pi_s + out.pi_0;
  out.e_mean = (out.z > 0.) ? ew / out.z : 0.;
  return out;
}

static void draw_group(mt19937 &rng, size_t n, vector<double> &eta) {
    uniform_real_distribution<double> unif(0., 1.);
    eta.resize(n);
    for (size_t b = 0; b < n; ++b) eta[b] = unif(rng);
}

/*The hard factors that the callers pass to thermo_star.*/
static void hard_products(const vector<double> &es, const vector<double> &eu,
                          double &A0, double &B0) {
    A0 = 1.;
    B0 = 1.;
    for (size_t b = 0; b < es.size(); ++b) A0 *= 1. - es[b];
    for (size_t b = 0; b < eu.size(); ++b) B0 *= 1. - eu[b];
}

static void test_matches_brute_force(mt19937 &rng) {
    const double temps[] = {0., 0.05, 0.3, 1., 5.};
    uniform_int_distribution<int> deg(0, 5);
    for (int trial = 0; trial < 200; ++trial) {
        vector<double> es, eu;
        draw_group(rng, (size_t)deg(rng), es);
        draw_group(rng, (size_t)deg(rng), eu);
        double A0, B0;
        hard_products(es, eu, A0, B0);
        vector<double> j0(es.size()), u0(eu.size());
        for (size_t b = 0; b < es.size(); ++b) j0[b] = 1. - es[b];
        for (size_t b = 0; b < eu.size(); ++b) u0[b] = 1. - eu[b];
        for (size_t k = 0; k < sizeof(temps) / sizeof(temps[0]); ++k) {
            ThermoStar got = thermo_star(es.data(), es.size(), eu.data(), eu.size(),
                                         A0, B0, temps[k]);
            ThermoStar want = brute_star(es, j0, eu, u0, temps[k]);
            expect_close("brute pi_u", got.pi_u, want.pi_u, 1e-12);
            expect_close("brute pi_s", got.pi_s, want.pi_s, 1e-12);
            expect_close("brute pi_0", got.pi_0, want.pi_0, 1e-12);
            expect_close("brute z", got.z, want.z, 1e-12);
            expect_close("brute e_mean", got.e_mean, want.e_mean, 1e-12);
        }
    }
}

/*The hard SP factors of Graph::__pu and Graph::__norm (rho_SP=1). The finite-T
 corrections underflow away, so tiny T returns the hard factors exactly.*/
static void test_zero_temperature_limit(mt19937 &rng) {
    uniform_int_distribution<int> deg(0, 8);
    for (int trial = 0; trial < 200; ++trial) {
        vector<double> es, eu;
        draw_group(rng, (size_t)deg(rng), es);
        draw_group(rng, (size_t)deg(rng), eu);
        double A0, B0;
        hard_products(es, eu, A0, B0);
        double temps[] = {0., 1e-9, 1e-6};
        for (size_t k = 0; k < 3; ++k) {
            ThermoStar got = thermo_star(es.data(), es.size(), eu.data(), eu.size(),
                                         A0, B0, temps[k]);
            expect_close("T=0 pi_u", got.pi_u, (1. - B0) * A0,
                         1e-14 * (1. + fabs(A0) + fabs(B0)));
            expect_close("T=0 pi_s", got.pi_s, (1. - A0) * B0,
                         1e-14 * (1. + fabs(A0) + fabs(B0)));
            expect_close("T=0 pi_0", got.pi_0, A0 * B0,
                         1e-14 * (1. + fabs(A0) + fabs(B0)));
            expect_close("T=0 norm", got.z, A0 + B0 - A0 * B0,
                         1e-14 * (1. + fabs(A0) + fabs(B0)));
        }
    }
}

static void test_partition_sum(mt19937 &rng) {
    const double temps[] = {0., 0.2, 3.};
    uniform_int_distribution<int> deg(0, 6);
    for (int trial = 0; trial < 200; ++trial) {
        vector<double> es, eu;
        draw_group(rng, (size_t)deg(rng), es);
        draw_group(rng, (size_t)deg(rng), eu);
        double A0, B0;
        hard_products(es, eu, A0, B0);
        for (size_t k = 0; k < sizeof(temps) / sizeof(temps[0]); ++k) {
            ThermoStar got = thermo_star(es.data(), es.size(), eu.data(), eu.size(),
                                         A0, B0, temps[k]);
            expect_close("z normalization", got.z,
                         got.pi_u + got.pi_s + got.pi_0, 1e-12);
        }
    }
}

/*The Gibbs draw of the decision policy: the frequency of true must match the
 logistic weight of the free-energy bias, and T_act=0 gives the hard rule.*/
static void test_gibbs_sampler() {
    srandom(20260922u);
    const double phi_plus[] = {0.3, 1.2, 0.7, 0.3};
    const double phi_minus[] = {1.2, 0.3, 0.7, 1.2};
    const double t_act[] = {0.5, 0.5, 0.3, 0.05};
    for (size_t k = 0; k < 4; ++k) {
        double p_true = 1. / (1. + exp((phi_plus[k] - phi_minus[k]) / t_act[k]));
        int hits = 0;
        for (int i = 0; i < 10000; ++i)
            if (thermo_gibbs_direction(phi_plus[k], phi_minus[k], t_act[k])) ++hits;
        expect_close("gibbs frequency", hits / 10000., p_true, 0.02);
    }
    expect_close("hard rule true", thermo_gibbs_direction(0.2, 0.5, 0.) ? 1. : 0.,
                 1., 0.);
    expect_close("hard rule false", thermo_gibbs_direction(0.5, 0.2, 0.) ? 1. : 0.,
                 0., 0.);
    expect_close("hard rule tie", thermo_gibbs_direction(0.3, 0.3, 0.) ? 1. : 0.,
                 0., 0.);
}

/*The unfrozen bias multiplies only the hard free mass A0*B0. Frozen sectors
 stay put, balanced conflicts at T>0 are not boosted, and eta=pi_u/z falls when
 that mass is positive.*/
static void test_rsb_gamma() {
    const double es[] = {0.2};
    const double eu[] = {0.3};
    const double A0 = 0.8;
    const double B0 = 0.7;
    const double hard = A0 * B0;
    const double gammas[] = {-0.4, 0., 0.5, 1.2};
    const double temps[] = {0., 0.25};
    for (size_t ig = 0; ig < 4; ++ig) {
        for (size_t it = 0; it < 2; ++it) {
            ThermoStar base = thermo_star(es, 1, eu, 1, A0, B0, temps[it], 0.);
            ThermoStar got = thermo_star(es, 1, eu, 1, A0, B0, temps[it], gammas[ig]);
            double boost = (gammas[ig] == 0.) ? 0. : (exp(gammas[ig]) - 1.) * hard;
            expect_close("gamma pi_u", got.pi_u, base.pi_u, 1e-12);
            expect_close("gamma pi_s", got.pi_s, base.pi_s, 1e-12);
            expect_close("gamma pi_0 boost", got.pi_0 - base.pi_0, boost, 1e-12);
            expect_close("gamma z boost", got.z - base.z, boost, 1e-12);
            expect_close("gamma z sum", got.z, got.pi_u + got.pi_s + got.pi_0, 1e-12);
            if (gammas[ig] == 0. && temps[it] == 0.) {
                expect_close("gamma0 pi_0", got.pi_0, hard, 1e-14);
                expect_close("gamma0 z", got.z, A0 + B0 - A0 * B0, 1e-14);
            }
        }
        ThermoStar hard0 = thermo_star(es, 1, eu, 1, A0, B0, 0., 0.);
        ThermoStar hardg = thermo_star(es, 1, eu, 1, A0, B0, 0., gammas[ig]);
        if (gammas[ig] > 0. && hard0.pi_u > 0. && hard0.z > 0. && hardg.z > 0.) {
            double eta0 = hard0.pi_u / hard0.z;
            double etag = hardg.pi_u / hardg.z;
            if (!(etag < eta0)) {
                fprintf(stderr, "FAIL gamma eta: %.17g not below %.17g\n", etag, eta0);
                ++failures;
            }
        }
    }
}

int main() {
    mt19937 rng(20260922u);
    test_matches_brute_force(rng);
    test_zero_temperature_limit(rng);
    test_partition_sum(rng);
    test_gibbs_sampler();
    test_rsb_gamma();
    if (failures == 0) printf("bsp-test: all thermo_sp identities hold\n");
    else printf("bsp-test: %d failures\n", failures);
    return failures == 0 ? 0 : 1;
}
