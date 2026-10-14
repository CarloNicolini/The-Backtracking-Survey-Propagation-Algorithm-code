//
//  sp2.cpp
//  BSP
//
//  Population update of the finite-mu 2RSB equations on the residual formula.
//

#include <bsp/sp2.hpp>

#include <bsp/Header.hpp>
#include <bsp/Logger.hpp>
#include <bsp/thermo_sp.hpp>

#include "rsb2_edges.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>

using namespace std;

double g_sp2_mu2 = 0.;
double g_sp2_x = 0.5;
unsigned g_sp2_pop = 32;
unsigned g_sp2_sweeps = 4;
unsigned g_sp2_every = 0;

void sp2_nu(const double *es, size_t ns, const double *eu, size_t nu, double mu, double &z,
            double &out) {
    double A0 = 1., B0 = 1.;
    for (size_t i = 0; i < ns; ++i) A0 *= 1. - es[i];
    for (size_t i = 0; i < nu; ++i) B0 *= 1. - eu[i];
    const ThermoStar st = thermo_star(es, ns, eu, nu, A0, B0, 1. / mu, 0.);
    z = st.z;
    out = (z > 0.) ? st.pi_u / z : 0.;
}

static void lows(const vector<const double *> &P, size_t M0, vector<double> &lo) {
    lo.resize(P.size());
    for (size_t b = 0; b < P.size(); ++b)
        lo[b] = *min_element(P[b], P[b] + M0);
}

bool sp2_variable(const vector<const double *> &S, const vector<const double *> &U, size_t M0,
                  double mu, double x, unsigned M, unsigned budget, mt19937_64 &rng,
                  unsigned &drawn, double *out) {
    vector<double> loS, loU, es(S.size()), eu(U.size());
    lows(S, M0, loS);
    lows(U, M0, loU);
    double zW, discard;
    sp2_nu(loS.data(), loS.size(), loU.data(), loU.size(), mu, zW, discard);
    drawn = 0;
    if (!(M0 > 0)) return false;
    if (!(zW > 0.)) {
        fill_n(out, M, 0.);
        return true;
    }
    uniform_int_distribution<size_t> pick(0, M0 - 1);
    uniform_real_distribution<double> unif(0., 1.);
    const double logW = x * log(zW);
    unsigned fill = 0;
    while (fill < M && drawn < budget) {
        for (size_t b = 0; b < S.size(); ++b) es[b] = S[b][pick(rng)];
        for (size_t b = 0; b < U.size(); ++b) eu[b] = U[b][pick(rng)];
        double z, nu;
        sp2_nu(es.data(), es.size(), eu.data(), eu.size(), mu, z, nu);
        ++drawn;
        if (!(z > 0.)) continue;
        const double ratio = exp(x * log(z) - logW);
        if (ratio > 1. + 1e-8) break;
        if (unif(rng) < min(ratio, 1.)) out[fill++] = nu;
    }
    if (fill < M) fill_n(out + fill, M - fill, 0.);
    return fill == M;
}

static void gather(const Rsb2Edges &E, const vector<vector<double>> &pop, unsigned group,
                   size_t skip, vector<const double *> &out) {
    out.clear();
    for (unsigned i = E.g[group]; i < E.g[group + 1]; ++i) {
        const size_t e = E.in[i];
        if (e != skip) out.push_back(pop[e].data());
    }
}

Sp2Obs sp2_run(Graph &G, double mu2, double x, unsigned pop, unsigned sweeps) {
    const Rsb2Edges E = rsb2_edges(G);
    const size_t ne = E.slot.size();
    mt19937_64 rng(g_fixed_seed >= 1 ? g_fixed_seed : random_device{}());
    uniform_real_distribution<double> unif(0., 1.);
    vector<vector<double>> cur(ne, vector<double>(pop)), nxt(ne, vector<double>(pop));
    for (size_t e = 0; e < ne; ++e)
        for (unsigned k = 0; k < pop; ++k) cur[e][k] = unif(rng) < *E.slot[e] ? 1. : 0.;
    unsigned drawn = 0, trunc = 0;
    const unsigned budget = max(64u * pop, 1024u);
    vector<const double *> S, U;
    vector<vector<double>> nu;
    for (unsigned t = 0; t < sweeps; ++t) {
        bsp::check_cancelled();
        for (size_t c = 0; c + 1 < E.cl.size(); ++c) {
            const unsigned f = E.cl[c], k = E.cl[c + 1] - f;
            nu.assign(k, vector<double>(pop, 0.));
            for (unsigned l = 0; l < k; ++l) {
                gather(E, cur, E.grp[f + l], f + l, S);
                gather(E, cur, E.grp[f + l] ^ 1, ne, U);
                unsigned d = 0;
                if (!sp2_variable(S, U, pop, mu2, x, pop, budget, rng, d, nu[l].data()))
                    ++trunc;
                drawn += d;
            }
            for (unsigned tgt = 0; tgt < k; ++tgt)
                for (unsigned s = 0; s < pop; ++s) {
                    double p = 1.;
                    for (unsigned l = 0; l < k; ++l)
                        if (l != tgt) p *= nu[l][s];
                    nxt[f + tgt][s] = p;
                }
        }
        cur.swap(nxt);
    }
    double mean = 0., spread = 0., diff = 0.;
    unsigned alive = 0;
    for (size_t e = 0; e < ne; ++e) {
        double m = 0., m2 = 0.;
        for (double y : cur[e]) {
            m += y;
            m2 += y * y;
        }
        m /= pop;
        mean += m;
        const double v = m2 / pop - m * m;
        if (m * (1. - m) > 1e-12) {
            spread += v / (m * (1. - m));
            ++alive;
        }
        diff += fabs(m - *E.slot[e]);
    }
    if (auto *ctx=bsp::run_context()) if(ctx->options->population_history && (ctx->sp2_step-1) % ctx->options->diag_every == 0) bsp::record_populations(G,ctx->sp2_step-1,cur);
    const double n = ne ? (double)ne : 1.;
    const double kept = ne && sweeps ? (double)ne * sweeps * pop : 1.;
    return {mean / n, alive ? spread / alive : 1., diff / n, drawn / kept, trunc};
}

void sp2_step(Graph &G) {
    static unsigned calls = 0;
    auto *ctx=bsp::run_context();
    const unsigned n = ctx ? ctx->sp2_step++ : calls++;
    if (g_sp2_every == 0 ? n > 0 : n % g_sp2_every != 0) return;
    const Sp2Obs o = sp2_run(G, g_sp2_mu2, g_sp2_x, g_sp2_pop, g_sp2_sweeps);
    if(ctx) {
        auto &table=ctx->result->tables["sp2"];
        table.columns={"step","mu2","x","pop","sweeps","mean","spread","diff","drawn_per_keep","trunc"};
        table.rows.push_back({double(n),g_sp2_mu2,g_sp2_x,double(g_sp2_pop),double(g_sp2_sweeps),o.mean,o.spread,o.diff,o.drawn_per_keep,double(o.trunc)});
        BSP_INFO << "sp2: mu2=" << g_sp2_mu2 << " x=" << g_sp2_x << " mean=" << o.mean
                 << " spread=" << o.spread << " |mean-sp|=" << o.diff << " trunc=" << o.trunc << endl;
        return;
    }
    const string name = (g_diag_prefix.empty() ? string() : g_diag_prefix + "_") + "sp2.csv";
    const bool fresh = !ifstream(name).good();
    ofstream csv(name, ios::app);
    csv << setprecision(10);
    if (fresh)
        csv << "step,mu2,x,pop,sweeps,mean,spread,diff,drawn_per_keep,trunc\n";
    csv << n << ',' << g_sp2_mu2 << ',' << g_sp2_x << ',' << g_sp2_pop << ',' << g_sp2_sweeps
        << ',' << o.mean << ',' << o.spread << ',' << o.diff << ',' << o.drawn_per_keep << ','
        << o.trunc << '\n';
    BSP_INFO << "sp2: mu2=" << g_sp2_mu2 << " x=" << g_sp2_x << " mean=" << o.mean
             << " spread=" << o.spread << " |mean-sp|=" << o.diff << " trunc=" << o.trunc << endl;
}
