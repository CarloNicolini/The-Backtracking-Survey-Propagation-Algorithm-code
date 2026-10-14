//
//  rsb2_thermo.cpp
//  BSP
//
//  2RSB linear stability test at an SP-y fixed point (ThermoSP with
//  y = 1/T_cav). See notes/2rsb_sp_ksat.md and bsp-lean/BspLean/Thermo*.
//

#include <bsp/rsb2.hpp>
#include <bsp/thermo_sp.hpp>

#include "rsb2_edges.hpp"

#include <cmath>

using namespace std;

namespace {
/*Coefficients of the group of edges [r0, r1) of E.in, without the edges a and b.*/
void group_coef(const Rsb2Edges &E, unsigned r0, unsigned r1, size_t a, size_t b,
                vector<double> &eta, vector<double> &coef) {
    eta.clear();
    for (unsigned r = r0; r < r1; ++r)
        if (E.in[r] != a && E.in[r] != b) eta.push_back(*E.slot[E.in[r]]);
    coef.resize(eta.size() + 1);
    thermo_coefficients(eta.data(), eta.size(), coef.data());
}
}

/*For the edge e = a->j the cavity star of j has the same-sign group S (grp[e]
 without e) and the opposite-sign group U (grp[e]^1), with coefficients cS and
 cU. A bug on an input b moves nu_j only in two channels (Lean: flipU_moves,
 flipS_moves). Channel U, b in U with p' = q' = k: the width passes with the
 weight e^(-yk) (nu_bug_U). Channel S, b in S with q = p'+1: the width passes
 from corner to corner with the weights e^(-yk) and e^(-y(k+1)) times the
 slopes e^(-mu2) and e^(mu2) (nu_bug_S, nuS_hasDerivAt_zero/one). With the
 corner-1 widths scaled by e^(mu2 - y/2) both S weights are e^(-y(k+1/2)), and
 the two corners split into sum and difference, the sum being the largest
 (charpoly_gauge, charpoly_symmetric). So one width per edge, with the source
 h_e = sum_b c_b w_b over the inputs of j and
   c_b = sum_k cS_k cU^{-b}_k e^(-yk) / z_e             for b in U,
   c_b = sum_k cS^{-b}_k cU_{k+1} e^(-y(k+1/2)) / z_e   for b in S.
 At y = inf only the k = 0 term of the channel U is left, A B'/N: rsb2_lambda.*/
double rsb2_lambda_y(Graph &G, double y) {
    const Rsb2Edges E = rsb2_edges(G);
    const size_t ne = E.slot.size();
    vector<double> nu(ne), coef, eta, cs, cu, cx, boltz;
    vector<size_t> src, off(1, 0);
    for (size_t e = 0; e < ne; ++e) {
        const unsigned q = E.grp[e], o = q ^ 1;
        group_coef(E, E.g[q], E.g[q + 1], e, e, eta, cs);
        group_coef(E, E.g[o], E.g[o + 1], e, e, eta, cu);
        const size_t ns = cs.size() - 1, nq = cu.size() - 1;
        const ThermoStar st = thermo_star_coef(cs.data(), ns, cu.data(), nq, cs[0], cu[0], 1. / y);
        nu[e] = st.pi_u / st.z;
        while (boltz.size() <= ns + nq) boltz.push_back(exp(-y * boltz.size()));
        for (unsigned r = E.g[o]; r < E.g[o + 1]; ++r) {
            group_coef(E, E.g[o], E.g[o + 1], E.in[r], E.in[r], eta, cx);
            double c = 0.;
            for (size_t k = 0; k < cx.size() && k <= ns; ++k) c += cs[k] * cx[k] * boltz[k];
            src.push_back(E.in[r]);
            coef.push_back(c / st.z);
        }
        for (unsigned r = E.g[q]; r < E.g[q + 1]; ++r) {
            if (E.in[r] == e) continue;
            group_coef(E, E.g[q], E.g[q + 1], e, E.in[r], eta, cx);
            double c = 0.;
            for (size_t k = 0; k < cx.size() && k < nq; ++k) c += cx[k] * cu[k + 1] * boltz[k];
            src.push_back(E.in[r]);
            coef.push_back(c * exp(-0.5 * y) / st.z);
        }
        off.push_back(src.size());
    }
    vector<double> h(ne);
    return rsb2_power(ne, [&](const vector<double> &w, vector<double> &nw) {
        for (size_t e = 0; e < ne; ++e) {
            h[e] = 0.;
            for (size_t r = off[e]; r < off[e + 1]; ++r) h[e] += coef[r] * w[src[r]];
        }
        for (size_t c = 0; c + 1 < E.cl.size(); ++c) {
            const unsigned f = E.cl[c];
            rsb2_spread_y(&h[f], &nu[f], E.cl[c + 1] - f, &nw[f]);
        }
    });
}
