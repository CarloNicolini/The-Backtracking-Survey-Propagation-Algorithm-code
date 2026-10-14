//
//  rsb2_pop.cpp
//  BSP
//
//  Population solver of the zero-energy 2RSB equations on the residual
//  formula. Each active edge keeps a two-class population of eta, one value
//  per cluster (rsb2_edges.hpp). The populations stay in memory between
//  calls, keyed by the eta slot, so every call starts from the previous one
//  (warm start). A new edge starts at the corner embedding R2 with the mean
//  of its SP message.
//

#include <bsp/rsb2.hpp>

#include "rsb2_edges.hpp"

#include <cmath>
#include <unordered_map>

using namespace std;

namespace {
/*Width given back to every member closer than this to a corner before the
 sweeps, so that a new instability can grow after a stable stretch.*/
const double k_seed_width = 1e-4;
/*probability of the low class and the members, per eta slot*/
unordered_map<const double *, pair<double, vector<double> > > g_pops;
mt19937_64 rng;
bool rng_ready=false;
}

Rsb2Obs rsb2_populations(Graph &G, double x, unsigned pop, unsigned sweeps) {
    if(!rng_ready) { rng.seed(g_fixed_seed >= 1 ? g_fixed_seed : random_device{}()); rng_ready=true; }
    uniform_int_distribution<unsigned> pick(0, pop - 1);
    uniform_real_distribution<double> unif(0., 1.);
    const Rsb2Edges E = rsb2_edges(G);
    const size_t ne = E.slot.size(), nc = E.cl.size() - 1;
    const unsigned lo = pop / 2;
    Rsb2Pops P = {E, vector<double *>(ne), vector<double>(ne), pop, rng};
    for (size_t e = 0; e < ne; ++e) {
        pair<double, vector<double> > &p = g_pops[E.slot[e]];
        if (p.second.size() != pop) {
            p.first = 1. - *E.slot[e];
            p.second.assign(pop, 1.);
            fill(p.second.begin(), p.second.begin() + lo, 0.);
        }
        if (sweeps)
            for (double &y : p.second) y = min(max(y, k_seed_width), 1. - k_seed_width);
        P.m[e] = p.second.data();
        P.f[e] = p.first;
    }
    /*mean of min(eta, 1-eta): eta for the low members, 1-eta for the high ones*/
    auto width = [&]() {
        double s = 0.;
        for (size_t e = 0; e < ne; ++e) {
            double sl = 0., sh = 0.;
            for (unsigned k = 0; k < lo; ++k) sl += P.m[e][k];
            for (unsigned k = lo; k < pop; ++k) sh += 1. - P.m[e][k];
            s += P.f[e] * sl / lo + (1. - P.f[e]) * sh / (pop - lo);
        }
        return s / ne;
    };

    /*Per clause: the cavity law Q_l of every variable, reweighted by N^x, as
     r[l] = Q_l(rho >= 1/2) and pop members of each class (qh, ql). The
     message to target i is prod_{l!=i} rho_l. It can be high only if every
     rho_l is high (probability H = prod r_l), and then it is high for the
     fraction phi of those products that stay >= 1/2. So the low class has
     probability 1 - H phi, and its members come from the other products and
     from the draws with at least one low rho_l (first low l with
     probability (1-r_l)/(1-suffix)).*/
    vector<double> cA, cB, cw, cm, crho, cnx, cz, qh, ql, r, suf, hi, lh, ll;
    Rsb2Obs o;
    o.width.push_back(width());
    for (unsigned t = 0; t < sweeps; ++t) {
        bsp::check_cancelled();
        for (size_t c = 0; c < nc; ++c) {
            const unsigned f = E.cl[c], k = E.cl[c + 1] - f;
            qh.resize(pop * k);
            ql.resize(pop * k);
            r.assign(k, 0.);
            vector<char> ok(k);
            for (unsigned l = 0; l < k; ++l) {
                cA.clear();
                cB.clear();
                cw.clear();
                rsb2_candidates(P, f + l, cA, cB, cw);
                const unsigned n = cA.size();
                crho.resize(n);
                cnx.resize(n);
                cz.resize(n);
                cm.resize(n);
                rsb2_star(cA.data(), cB.data(), 1, n, x, crho.data(), cnx.data(), cz.data());
                double wt = 0., wh = 0.;
                for (unsigned s = 0; s < n; ++s) {
                    cw[s] *= cnx[s];
                    wt += cw[s];
                    if (crho[s] >= 0.5) wh += cw[s];
                }
                ok[l] = wt > 0.;
                if (!ok[l]) continue;
                r[l] = wh / wt;
                for (int high = 0; high < 2; ++high) {
                    for (unsigned s = 0; s < n; ++s) cm[s] = ((crho[s] >= 0.5) == bool(high)) ? cw[s] : 0.;
                    rsb2_resample(crho.data(), cm.data(), n, unif(rng), &(high ? qh : ql)[l * pop], pop);
                }
            }
            for (unsigned i = 0; i < k; ++i) {
                bool alive = true;
                for (unsigned l = 0; l < k; ++l) alive = alive && (l == i || ok[l]);
                if (!alive) continue;
                suf.assign(k + 1, 1.);
                for (unsigned l = k; l-- > 0;) suf[l] = suf[l + 1] * ((l == i) ? 1. : r[l]);
                const double H = suf[0];
                hi.clear();
                lh.clear();
                ll.clear();
                for (unsigned s = 0; s < pop; ++s) {
                    double eta = 1.;
                    for (unsigned l = 0; l < k; ++l)
                        if (l != i) eta *= qh[l * pop + pick(rng)];
                    (eta >= 0.5 ? hi : lh).push_back(eta);
                }
                for (unsigned s = 0; s < pop && H < 1.; ++s) {
                    double eta = 1.;
                    bool need_low = true;
                    for (unsigned l = 0; l < k; ++l) {
                        if (l == i) continue;
                        const bool low = need_low ? unif(rng) * (1. - suf[l]) < 1. - r[l]
                                                  : unif(rng) >= r[l];
                        need_low = need_low && !low;
                        eta *= (low ? ql : qh)[l * pop + pick(rng)];
                    }
                    ll.push_back(eta);
                }
                const double phi = double(hi.size()) / pop, fo = 1. - H * phi;
                const double plh = (fo > 0.) ? H * (1. - phi) / fo : 0.;
                auto any = [&](const vector<double> &v) {
                    return v[uniform_int_distribution<size_t>(0, v.size() - 1)(rng)];
                };
                double *out = P.m[f + i];
                for (unsigned s = lo; s < pop; ++s) out[s] = hi.empty() ? 1. : any(hi);
                for (unsigned s = 0; s < lo; ++s) {
                    const bool from_lh = !lh.empty() && (ll.empty() || unif(rng) < plh);
                    out[s] = from_lh ? any(lh) : (ll.empty() ? 0. : any(ll));
                }
                P.f[f + i] = fo;
            }
        }
        o.width.push_back(width());
    }
    for (size_t e = 0; e < ne; ++e) g_pops[E.slot[e]].first = P.f[e];
    if (auto *ctx=bsp::run_context()) if(ctx->options->population_history && (ctx->rsb2_step-1) % ctx->options->diag_every == 0) {
        std::vector<std::vector<double>> out;
        for(size_t e=0;e<ne;++e) out.emplace_back(P.m[e],P.m[e]+pop);
        bsp::record_populations(G,ctx->rsb2_step-1,std::move(out));
    }
    rsb2_observe(P, x, o);
    return o;
}

void bsp::reset_rsb2_populations() { g_pops.clear(); rng_ready=false; }
