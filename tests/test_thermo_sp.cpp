//
//  test_thermo_sp.cpp
//  BSP tests
//
//  Numeric identities of the thermodynamic star sums (thermodynamic-sp.md):
//  1. the sums match a brute-force enumeration of the warning configurations;
//  2. the T=0 and tiny-T sums match the hard SP factors;
//  3. the partition sum z equals pi_u+pi_s+pi_0.
//

#include "thermo_sp.hpp"

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
 weight it with its prior times e^(-min(p,q)/T), and sort it by region.*/
static ThermoStar brute_star(const vector<double> &es, const vector<double> &eu,
                             double T) {
    ThermoStar out = {0., 0., 0., 0.};
    const size_t ns = es.size(), nu = eu.size();
    const size_t ms = 1u << ns, mu = 1u << nu;
    for (size_t xs = 0; xs < ms; ++xs) {
        for (size_t xu = 0; xu < mu; ++xu) {
            size_t p = 0, q = 0;
            double prob = 1.;
            for (size_t b = 0; b < ns; ++b) {
                if (xs & (1u << b)) { ++p; prob *= es[b]; }
                else prob *= 1. - es[b];
            }
            for (size_t b = 0; b < nu; ++b) {
                if (xu & (1u << b)) { ++q; prob *= eu[b]; }
                else prob *= 1. - eu[b];
            }
            double conflict = (p < q) ? (double)p : (double)q;
            double w = (T > 0.) ? prob * exp(-conflict / T)
                                : ((conflict == 0.) ? prob : 0.);
            if (q > p) out.pi_u += w;
            else if (p > q) out.pi_s += w;
            else out.pi_0 += w;
        }
    }
    out.z = out.pi_u + out.pi_s + out.pi_0;
    return out;
}

static void draw_group(mt19937 &rng, size_t n, vector<double> &eta) {
    uniform_real_distribution<double> unif(0., 1.);
    eta.resize(n);
    for (size_t b = 0; b < n; ++b) eta[b] = unif(rng);
}

static void test_matches_brute_force(mt19937 &rng) {
    const double temps[] = {0., 0.05, 0.3, 1., 5.};
    uniform_int_distribution<int> deg(0, 5);
    for (int trial = 0; trial < 200; ++trial) {
        vector<double> es, eu;
        draw_group(rng, (size_t)deg(rng), es);
        draw_group(rng, (size_t)deg(rng), eu);
        for (size_t k = 0; k < sizeof(temps) / sizeof(temps[0]); ++k) {
            ThermoStar got = thermo_star(es.data(), es.size(), eu.data(), eu.size(),
                                         temps[k]);
            ThermoStar want = brute_star(es, eu, temps[k]);
            expect_close("brute pi_u", got.pi_u, want.pi_u, 1e-12);
            expect_close("brute pi_s", got.pi_s, want.pi_s, 1e-12);
            expect_close("brute pi_0", got.pi_0, want.pi_0, 1e-12);
        }
    }
}

/*The hard SP factors of Graph::__pu and Graph::__norm (rho_SP=1).*/
static void test_zero_temperature_limit(mt19937 &rng) {
    uniform_int_distribution<int> deg(0, 8);
    for (int trial = 0; trial < 200; ++trial) {
        vector<double> es, eu;
        draw_group(rng, (size_t)deg(rng), es);
        draw_group(rng, (size_t)deg(rng), eu);
        double A0 = 1., B0 = 1.;
        for (size_t b = 0; b < es.size(); ++b) A0 *= 1. - es[b];
        for (size_t b = 0; b < eu.size(); ++b) B0 *= 1. - eu[b];
        double temps[] = {0., 1e-9};
        for (size_t k = 0; k < 2; ++k) {
            ThermoStar got = thermo_star(es.data(), es.size(), eu.data(), eu.size(),
                                         temps[k]);
            expect_close("T=0 pi_u", got.pi_u, (1. - B0) * A0, 1e-12);
            expect_close("T=0 pi_s", got.pi_s, (1. - A0) * B0, 1e-12);
            expect_close("T=0 pi_0", got.pi_0, A0 * B0, 1e-12);
            expect_close("T=0 norm", got.z, A0 + B0 - A0 * B0, 1e-12);
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
        for (size_t k = 0; k < sizeof(temps) / sizeof(temps[0]); ++k) {
            ThermoStar got = thermo_star(es.data(), es.size(), eu.data(), eu.size(),
                                         temps[k]);
            expect_close("z normalization", got.z,
                         got.pi_u + got.pi_s + got.pi_0, 1e-12);
        }
    }
}

int main() {
    mt19937 rng(20260922u);
    test_matches_brute_force(rng);
    test_zero_temperature_limit(rng);
    test_partition_sum(rng);
    if (failures == 0) printf("bsp-test: all thermo_sp identities hold\n");
    else printf("bsp-test: %d failures\n", failures);
    return failures == 0 ? 0 : 1;
}
