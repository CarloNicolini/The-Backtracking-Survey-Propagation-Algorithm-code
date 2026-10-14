//
//  rsb2_draw.cpp
//  BSP
//
//  Draws from the two-class populations of the 2RSB solver (rsb2_edges.hpp):
//  single members, products over a group, and the stratified candidates of
//  a cavity.
//

#include "rsb2_edges.hpp"

using namespace std;

double rsb2_member(Rsb2Pops &P, size_t e) {
    const unsigned lo = P.pop / 2;
    if (uniform_real_distribution<double>(0., 1.)(P.rng) < P.f[e])
        return P.m[e][uniform_int_distribution<unsigned>(0, lo - 1)(P.rng)];
    return P.m[e][uniform_int_distribution<unsigned>(lo, P.pop - 1)(P.rng)];
}

double rsb2_draw(Rsb2Pops &P, unsigned q, size_t skip) {
    double p = 1.;
    for (unsigned r = P.E.g[q]; r < P.E.g[q + 1]; ++r)
        if (P.E.in[r] != skip) p *= 1. - rsb2_member(P, P.E.in[r]);
    return p;
}

namespace {
/*suf[j] is the probability that the edges of group g from the j-th on are
 all low, so suf[0] is the probability of the low stratum.*/
double suffix_low(const Rsb2Pops &P, unsigned g, size_t skip, vector<double> &suf) {
    const Rsb2Edges &E = P.E;
    suf.assign(E.g[g + 1] - E.g[g] + 1, 1.);
    for (unsigned r = E.g[g + 1]; r-- > E.g[g];)
        suf[r - E.g[g]] = suf[r - E.g[g] + 1] * ((E.in[r] == skip) ? 1. : P.f[E.in[r]]);
    return suf[0];
}

/*prod (1-eta) over group g, conditioned on the low stratum or on its
 complement. The first high edge j has probability (1-f_j)/(1-suf[j]).*/
double draw_cond(Rsb2Pops &P, unsigned g, size_t skip, bool low, const vector<double> &suf) {
    const Rsb2Edges &E = P.E;
    const unsigned lo = P.pop / 2;
    uniform_real_distribution<double> unif(0., 1.);
    uniform_int_distribution<unsigned> pick_lo(0, lo - 1), pick_hi(lo, P.pop - 1);
    double p = 1.;
    bool need_high = !low;
    for (unsigned r = E.g[g]; r < E.g[g + 1]; ++r) {
        const unsigned e = E.in[r];
        if (e == skip) continue;
        double y;
        if (low || (need_high && unif(P.rng) * (1. - suf[r - E.g[g]]) >= 1. - P.f[e])) {
            y = P.m[e][pick_lo(P.rng)];
        } else if (need_high) {
            y = P.m[e][pick_hi(P.rng)];
            need_high = false;
        } else {
            y = rsb2_member(P, e);
        }
        p *= 1. - y;
    }
    return p;
}
}

void rsb2_candidates(Rsb2Pops &P, size_t e, vector<double> &A, vector<double> &B,
                     vector<double> &w) {
    const unsigned gs = P.E.grp[e], gu = gs ^ 1;
    const size_t none = P.E.slot.size();
    vector<double> sufS, sufU;
    const double ps = suffix_low(P, gs, e, sufS), pu = suffix_low(P, gu, none, sufU);
    for (int st = 0; st < 4; ++st) {
        const bool ls = st & 1, lu = st & 2;
        const double pst = (ls ? ps : 1. - ps) * (lu ? pu : 1. - pu);
        if (!(pst > 0.)) continue;
        for (unsigned s = 0; s < P.pop; ++s) {
            A.push_back(draw_cond(P, gs, e, ls, sufS));
            B.push_back(draw_cond(P, gu, none, lu, sufU));
            w.push_back(pst);
        }
    }
}
