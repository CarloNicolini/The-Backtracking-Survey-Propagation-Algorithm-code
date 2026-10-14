//
//  rsb2_graph.cpp
//  BSP
//
//  2RSB on the residual formula of Graph: the edges, the linearized
//  stability test around the corner embedding R2, and the per-step CSV.
//  See notes/2rsb_sp_ksat.md.
//

#include <bsp/rsb2.hpp>

#include "rsb2_edges.hpp"

#include <bsp/thermo_sp.hpp>

#include <cmath>
#include <fstream>
#include <numeric>
#include <limits>

using namespace std;

bool g_rsb2_stab = false;
double g_rsb2_x = 0.;
unsigned g_rsb2_pop = 64;
unsigned g_rsb2_every = 1;

namespace {
const unsigned k_power_iters = 2000;
const double k_power_tol = 1e-9;
const unsigned k_sweeps = 30; /*population sweeps per logged step*/
}

Rsb2Edges rsb2_edges(Graph &G) {
    Rsb2Edges E;
    const unsigned n = G.N();
    E.vtx.assign(n, NULL);
    E.cl.push_back(0);
    for (Clause *c : G._cl_list) {
        for (size_t l = 0; l < c->_go_forward.size(); ++l) {
            if (!c->_go_forward[l]) continue;
            Vertex *v = c->v_V[l];
            E.vtx[v->_vertex - 1] = v;
            E.slot.push_back(c->v_survey_cl_to_i[l]);
            E.grp.push_back(2 * (v->_vertex - 1) + (c->v_lit[l] ? 0 : 1));
        }
        E.cl.push_back(E.slot.size());
    }
    E.g.assign(2 * n + 1, 0);
    for (unsigned q : E.grp) ++E.g[q + 1];
    partial_sum(E.g.begin(), E.g.end(), E.g.begin());
    E.in.resize(E.slot.size());
    vector<unsigned> next(E.g.begin(), E.g.end() - 1);
    for (unsigned e = 0; e < E.slot.size(); ++e) E.in[next[E.grp[e]]++] = e;
    return E;
}

/*The log growth of the total width can alternate on bipartite-like
 structures, so the estimate is the mean of two consecutive steps.*/
double rsb2_power(size_t ne, const function<void(const vector<double> &, vector<double> &)> &apply) {
    vector<double> w(ne, 1.), nw(ne);
    double last = 0., est = 0.;
    for (unsigned it = 0; it < k_power_iters; ++it) {
        bsp::check_cancelled();
        apply(w, nw);
        const double s = accumulate(nw.begin(), nw.end(), 0.);
        const double l = log(s / accumulate(w.begin(), w.end(), 0.));
        if (!(s > 0.)) return l;
        const double mean = (it == 0) ? l : 0.5 * (l + last);
        last = l;
        for (double &v : nw) v *= ne / s;
        swap(w, nw);
        if (it > 0 && fabs(mean - est) < k_power_tol) return mean;
        est = mean;
    }
    BSP_DEBUG << "rsb2: power iteration stopped at " << k_power_iters << " steps" << endl;
    return est;
}

/*A width w_b of an input b->j in the opposite-sign group of j enters
 rsb2_spread as w_b/(1-eta_b), summed over the group.*/
double rsb2_lambda(Graph &G) {
    const Rsb2Edges E = rsb2_edges(G);
    const size_t ne = E.slot.size();
    vector<double> A(ne, 1.), B(ne, 1.);
    for (size_t e = 0; e < ne; ++e) {
        const unsigned q = E.grp[e], o = q ^ 1;
        for (unsigned r = E.g[q]; r < E.g[q + 1]; ++r)
            if (E.in[r] != e) A[e] *= 1. - *E.slot[E.in[r]];
        for (unsigned r = E.g[o]; r < E.g[o + 1]; ++r) B[e] *= 1. - *E.slot[E.in[r]];
    }
    vector<double> h(ne), hg(E.g.size());
    return rsb2_power(ne, [&](const vector<double> &w, vector<double> &nw) {
        fill(hg.begin(), hg.end(), 0.);
        for (size_t e = 0; e < ne; ++e) hg[E.grp[e]] += w[e] / (1. - *E.slot[e]);
        for (size_t e = 0; e < ne; ++e) h[e] = hg[E.grp[e] ^ 1];
        for (size_t c = 0; c + 1 < E.cl.size(); ++c) {
            const unsigned f = E.cl[c];
            rsb2_spread(&A[f], &B[f], &h[f], E.cl[c + 1] - f, &nw[f]);
        }
    });
}

void rsb2_step(Graph &G) {
    if(auto *ctx=bsp::run_context()) {
        const unsigned step=ctx->rsb2_step++;
        if(step % g_rsb2_every != 0) return;
        auto &table=ctx->result->tables["rsb2"];
        table.columns={"step","n_free","m_active","sigma_sp","backtrack","lambda","x","width0","width","lambda_pop","phi","s","sigma1","dmarg","spread"};
        const double nan=std::numeric_limits<double>::quiet_NaN();
        double lambda=nan;
        if(g_rsb2_stab) { lambda=g_T_cav>0.?rsb2_lambda_y(G,1./g_T_cav):rsb2_lambda(G); BSP_INFO<<"rsb2: lambda="<<lambda<<endl; }
        std::vector<bsp::Cell> row={double(step),double(G.N_t()),double(G._cl_list.size()),G.complexity,double(G.fl_bsp),g_rsb2_stab?bsp::Cell(lambda):bsp::Cell()};
        if(g_rsb2_x>0.) {
            auto o=rsb2_populations(G,g_rsb2_x,g_rsb2_pop,k_sweeps);
            row.insert(row.end(),{g_rsb2_x,o.width.front(),o.width.back(),log(o.width.back()/o.width[1])/(k_sweeps-1),o.phi,o.s,o.phi-g_rsb2_x*o.s,o.dmarg,o.spread});
            BSP_INFO << "rsb2: x=" << g_rsb2_x << " width=" << o.width.back() << " spread=" << o.spread
                     << " Sigma_1=" << o.phi-g_rsb2_x*o.s << " s=" << o.s << " Sigma_SP=" << G.complexity << endl;
        } else for(unsigned i=0;i<9;++i) row.emplace_back();
        table.rows.push_back(std::move(row)); return;
    }
    static unsigned step = 0;
    static ofstream csv;
    if (step++ % g_rsb2_every != 0) return;
    if (!csv.is_open()) {
        csv.open((g_diag_prefix.empty() ? string() : g_diag_prefix + "_") + "rsb2.csv");
        csv << "step,n_free,m_active,sigma_sp,backtrack,lambda,"
               "x,width0,width,lambda_pop,phi,s,sigma1,dmarg,spread\n";
        csv << setprecision(10);
    }
    csv << step - 1 << ',' << G.N_t() << ',' << G._cl_list.size() << ','
        << G.complexity << ',' << G.fl_bsp << ',';
    if (g_rsb2_stab) {
        const double lambda = (g_T_cav > 0.) ? rsb2_lambda_y(G, 1. / g_T_cav) : rsb2_lambda(G);
        csv << lambda;
        BSP_INFO << "rsb2: lambda=" << lambda << endl;
    }
    if (g_rsb2_x > 0.) {
        const Rsb2Obs o = rsb2_populations(G, g_rsb2_x, g_rsb2_pop, k_sweeps);
        const double w0 = o.width.front(), w1 = o.width.back();
        const double sigma1 = o.phi - g_rsb2_x * o.s;
        /*the first sweep is left out: at x < 1 it turns a seed d into width ~d^x*/
        csv << ',' << g_rsb2_x << ',' << w0 << ',' << w1 << ','
            << log(w1 / o.width[1]) / (k_sweeps - 1)
            << ',' << o.phi << ',' << o.s << ',' << sigma1 << ',' << o.dmarg << ',' << o.spread;
        BSP_INFO << "rsb2: x=" << g_rsb2_x << " width=" << w1 << " spread=" << o.spread
                 << " Sigma_1=" << sigma1 << " s=" << o.s << " Sigma_SP=" << G.complexity << endl;
    } else {
        csv << ",,,,,,,,,";
    }
    csv << endl;
}
