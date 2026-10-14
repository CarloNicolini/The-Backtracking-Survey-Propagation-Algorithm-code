//
//  rsb2_edges.hpp
//  BSP
//
//  Edges of the residual formula, shared by the 2RSB linear test and the
//  population solver, and the populations of the solver. Only public members
//  of Graph are read.
//

#ifndef BSP_RSB2_EDGES_HPP
#define BSP_RSB2_EDGES_HPP

#include <bsp/Graph.hpp>
#include <bsp/rsb2.hpp>

#include <functional>
#include <random>
#include <vector>

/*Active edges a->i of the unsatisfied clauses. slot[e] is the eta of edge e.
 The edges of the c-th clause are [cl[c], cl[c+1]). A group is the set of
 edges into variable v with one sign: 2v for the positive literals, 2v+1 for
 the negative ones. grp[e] is the group of edge e, so grp[e]^1 is the
 opposite-sign group. The edges of group q are in[g[q] .. g[q+1]).
 vtx[v] is the Vertex of index v, or NULL if v is in no active clause.*/
struct Rsb2Edges {
    std::vector<double *> slot;
    std::vector<unsigned> grp, cl, g, in;
    std::vector<Vertex *> vtx;
};

Rsb2Edges rsb2_edges(Graph &G);

/*Growth rate per sweep of w <- T w from w = 1 on ne edges, where apply(w, nw)
 writes T w into nw (rsb2_graph.cpp).*/
double rsb2_power(size_t ne,
                  const std::function<void(const std::vector<double> &, std::vector<double> &)> &apply);

/*Two-class populations. m[e][k] is member k of edge e. The first pop/2
 members are low (eta < 1/2), the others high, and f[e] is the probability
 of the low class. A real f keeps the corner embedding R2 exact for any eta
 (low members 0, high members 1, f = 1-eta), with no rounding of eta to a
 multiple of 1/pop.*/
struct Rsb2Pops {
    const Rsb2Edges &E;
    std::vector<double *> m;
    std::vector<double> f;
    unsigned pop;
    std::mt19937_64 &rng;
};

/*A random member of edge e, low with probability f[e].*/
double rsb2_member(Rsb2Pops &P, size_t e);

/*prod (1-eta) over group q, one random member per edge, edge skip excluded.*/
double rsb2_draw(Rsb2Pops &P, unsigned q, size_t skip);

/*Candidates (A, B) of the cavity of edge e's variable toward its clause,
 appended to A and B: pop draws in each of the four strata (every same-sign
 input low or not, every opposite-sign input low or not), with the stratum
 probability appended to w. So sum_s w[s] g(A_s, B_s) / pop estimates
 E[g]. At the corners only the low strata are free of contradictions, and
 their mass is exact.*/
void rsb2_candidates(Rsb2Pops &P, size_t e, std::vector<double> &A,
                     std::vector<double> &B, std::vector<double> &w);

/*Writes phi, s and dmarg of o for the populations of P (rsb2_obs.cpp).*/
void rsb2_observe(Rsb2Pops &P, double x, Rsb2Obs &o);

#endif /* BSP_RSB2_EDGES_HPP */
