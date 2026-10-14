//
//  rsb2_obs.cpp
//  BSP
//
//  Observables of the 2RSB population solver: Phi(x), s = Phi'(x) and the
//  change of the marginals with respect to SP.
//
//  Phi is in the Bethe form sum_a log E_Q[(1-prod_l rho_l)^x] + sum_i
//  log E[N_i^x] - sum_(ia) log E_Q[(1-eta_{a->i} rho_{i->a})^x], where Q is
//  the cavity law reweighted by N^x. It equals sum_a log E[Z_a^x] -
//  sum_i (d_i-1) log E[N_i^x], but its terms are O(1), so no factor d_i-1
//  amplifies the sampling noise. N, pi+ and the linear parts are
//  multilinear, so their means are exact from the population means
//  (Za_mean, R3_marginal). The stratified candidates, with self-normalized
//  weights, only estimate u^x - u, which vanishes at the corners and at
//  x = 1. s = Phi'(x) at fixed draws is a central difference
//  (hasDerivAt_Phi).
//

#include "rsb2_edges.hpp"

#include <cmath>

using namespace std;

void rsb2_observe(Rsb2Pops &P, double x, Rsb2Obs &o) {
    const Rsb2Edges &E = P.E;
    const size_t ne = E.slot.size(), nc = E.cl.size() - 1;
    const unsigned pop = P.pop, lo = pop / 2;
    vector<double> mu(ne);
    double var = 0., var_r2 = 0.;
    for (size_t e = 0; e < ne; ++e) {
        double sl = 0., sh = 0., ql = 0., qh = 0.;
        for (unsigned k = 0; k < lo; ++k) {
            sl += P.m[e][k];
            ql += P.m[e][k] * P.m[e][k];
        }
        for (unsigned k = lo; k < pop; ++k) {
            sh += P.m[e][k];
            qh += P.m[e][k] * P.m[e][k];
        }
        mu[e] = P.f[e] * sl / lo + (1. - P.f[e]) * sh / (pop - lo);
        var += P.f[e] * ql / lo + (1. - P.f[e]) * qh / (pop - lo) - mu[e] * mu[e];
        var_r2 += mu[e] * (1. - mu[e]);
    }
    o.spread = var / var_r2;
    auto mean_prod = [&](unsigned g, size_t skip) {
        double p = 1.;
        for (unsigned r = E.g[g]; r < E.g[g + 1]; ++r)
            if (E.in[r] != skip) p *= 1. - mu[E.in[r]];
        return p;
    };
    const double h = 1e-3 * x, xs[3] = {x - h, x, x + h};
    double phi3[3] = {0., 0., 0.};
    vector<double> cA, cB, cw, crho, cnx, cz, eta;
    vector<unsigned> off, tup;
    for (size_t c = 0; c < nc; ++c) {
        const unsigned f = E.cl[c], k = E.cl[c + 1] - f;
        cA.clear();
        cB.clear();
        cw.clear();
        eta.clear();
        off.assign(1, 0);
        for (unsigned l = 0; l < k; ++l) {
            rsb2_candidates(P, f + l, cA, cB, cw);
            while (eta.size() < cA.size()) eta.push_back(rsb2_member(P, f + l));
            off.push_back(cA.size());
        }
        /*random tuples, one candidate per variable, for the clause term*/
        tup.resize(pop * k);
        for (unsigned s = 0; s < pop; ++s)
            for (unsigned l = 0; l < k; ++l)
                tup[s * k + l] = uniform_int_distribution<unsigned>(off[l], off[l + 1] - 1)(P.rng);
        const unsigned n = cA.size();
        crho.resize(n);
        cnx.resize(n);
        cz.resize(n);
        for (int j = 0; j < 3; ++j) {
            rsb2_star(cA.data(), cB.data(), 1, n, xs[j], crho.data(), cnx.data(), cz.data());
            double pr = 1.;
            for (unsigned l = 0; l < k; ++l) {
                const double a = mean_prod(E.grp[f + l], f + l), b = mean_prod(E.grp[f + l] ^ 1, ne);
                double en = a + b - a * b, eu = (1. - b) * a, ce = 0., we = 0.;
                for (unsigned s = off[l]; s < off[l + 1]; ++s) {
                    const double w = cw[s] * cnx[s], t = 1. - eta[s] * crho[s];
                    en += cw[s] * (cnx[s] - (cA[s] + cB[s] - cA[s] * cB[s])) / pop;
                    eu += cw[s] * (cnx[s] * crho[s] - (1. - cB[s]) * cA[s]) / pop;
                    ce += w * (pow(t, xs[j]) - t);
                    we += w;
                }
                const double rb = (en > 0.) ? eu / en : 0.;
                pr *= rb;
                phi3[j] -= log(1. - mu[f + l] * rb + ((we > 0.) ? ce / we : 0.));
            }
            double cc = 0., wc = 0.;
            for (unsigned s = 0; s < pop; ++s) {
                double W = 1., g = 1.;
                for (unsigned l = 0; l < k; ++l) {
                    W *= cw[tup[s * k + l]] * cnx[tup[s * k + l]];
                    g *= crho[tup[s * k + l]];
                }
                cc += W * (pow(1. - g, xs[j]) - (1. - g));
                wc += W;
            }
            phi3[j] += log(1. - pr + ((wc > 0.) ? cc / wc : 0.));
        }
    }
    vector<double> Ns(pop), tT(pop), tF(pop);
    unsigned nv = 0;
    o.dmarg = 0.;
    for (unsigned v = 0; v < E.vtx.size(); ++v) {
        if (E.g[2 * v + 2] == E.g[2 * v]) continue;
        const double pb = mean_prod(2 * v, ne), mb = mean_prod(2 * v + 1, ne);
        for (unsigned s = 0; s < pop; ++s) {
            const double Pp = rsb2_draw(P, 2 * v, ne), M = rsb2_draw(P, 2 * v + 1, ne);
            Ns[s] = M + Pp - M * Pp;
            tT[s] = (Ns[s] > 0.) ? (1. - Pp) * M / Ns[s] : 0.;
            tF[s] = (Ns[s] > 0.) ? (1. - M) * Pp / Ns[s] : 0.;
        }
        for (int j = 0; j < 3; ++j) {
            double en = mb + pb - mb * pb, st = (1. - pb) * mb, sf = (1. - mb) * pb;
            for (unsigned s = 0; s < pop; ++s) {
                const double dn = (pow(Ns[s], xs[j]) - Ns[s]) / pop;
                en += dn;
                st += dn * tT[s];
                sf += dn * tF[s];
            }
            phi3[j] += log(en);
            if (j == 1) o.dmarg += fabs((st - sf) / en - (E.vtx[v]->_sT - E.vtx[v]->_sF));
        }
        ++nv;
    }
    o.phi = phi3[1];
    o.s = (phi3[2] - phi3[0]) / (2. * h);
    o.dmarg /= nv;
}
