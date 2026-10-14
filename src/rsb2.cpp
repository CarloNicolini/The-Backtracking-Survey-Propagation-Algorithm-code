//
//  rsb2.cpp
//  BSP
//
//  Pure kernels of the zero-energy 2RSB survey propagation (rsb2.hpp).
//

#include <bsp/rsb2.hpp>

#include <cmath>
#include <numeric>

using namespace std;

void rsb2_star(const double *A, const double *B, unsigned k, unsigned P,
               double x, double *rho, double *nx, double *z) {
    for (unsigned s = 0; s < P; ++s) {
        double pn = 1., pu = 1.;
        for (unsigned l = 0; l < k; ++l) {
            const double a = A[s * k + l], b = B[s * k + l];
            const double n = a + b - a * b, u = (1. - b) * a;
            rho[l * P + s] = (n > 0.) ? u / n : 0.;
            nx[l * P + s] = pow(n, x);
            pn *= n;
            pu *= u;
        }
        z[s] = pn - pu;
    }
}

bool rsb2_resample(const double *val, const double *w, unsigned n, double u,
                   double *out, unsigned P) {
    const double total = accumulate(w, w + n, 0.);
    if (!(total > 0.)) return false;
    unsigned i = 0;
    double c = w[0];
    for (unsigned k = 0; k < P; ++k) {
        const double t = (k + u) * total / P;
        while (c <= t && i + 1 < n) c += w[++i];
        out[k] = val[i];
    }
    return true;
}

void rsb2_spread(const double *A, const double *B, const double *h,
                 unsigned k, double *out) {
    for (unsigned t = 0; t < k; ++t) {
        out[t] = 0.;
        for (unsigned l = 0; l < k; ++l) {
            const double nl = A[l] + B[l] - A[l] * B[l];
            if (l == t || !(nl > 0.)) continue;
            double g = A[l] * B[l] * h[l] / nl;
            for (unsigned j = 0; j < k; ++j) {
                const double nj = A[j] + B[j] - A[j] * B[j];
                if (j != t && j != l) g *= (nj > 0.) ? (1. - B[j]) * A[j] / nj : 0.;
            }
            out[t] += g;
        }
    }
}

void rsb2_spread_y(const double *h, const double *nu, unsigned k, double *out) {
    for (unsigned t = 0; t < k; ++t) {
        out[t] = 0.;
        for (unsigned l = 0; l < k; ++l) {
            if (l == t) continue;
            double g = h[l];
            for (unsigned j = 0; j < k; ++j)
                if (j != t && j != l) g *= nu[j];
            out[t] += g;
        }
    }
}
