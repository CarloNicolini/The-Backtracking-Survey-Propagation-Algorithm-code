//
//  rsb2.hpp
//  BSP
//
//  Zero-energy 2RSB survey propagation for random K-SAT. See
//  notes/2rsb_sp_ksat.md for the derivation and bsp-lean for the proofs.
//  Each SP message eta_{a->i} becomes a population of P values, one per
//  cluster. The value of a cluster is the fraction of its sub-clusters in
//  which clause a warns i, and a cluster carries the weight N^x, with
//  x = mu1/mu2 in (0, 1].
//

#ifndef BSP_RSB2_HPP
#define BSP_RSB2_HPP

#include <vector>

class Graph;

/*P samples of the cavity stars of one clause with k variables. A[s*k+l] and
 B[s*k+l] are the products of (1-eta) over the same-sign and the
 opposite-sign inputs of variable l in sample s, as _prod_S and _prod_U. With
 N_l = A_l+B_l-A_l B_l, the call writes rho[l*P+s] = (1-B_l)A_l/N_l,
 nx[l*P+s] = N_l^x and z[s] = prod_l N_l - prod_l (1-B_l)A_l, the clause term
 of Phi. N_l = 0 gives rho_l = 0 and weight 0. The message to target t is
 prod_{l!=t} rho_l with weight prod_{l!=t} N_l^x (Lean: outMean, Za_mean).
 The weight factorizes, so the solver draws each rho_l from its own
 population reweighted by N_l^x and multiplies unweighted draws.*/
void rsb2_star(const double *A, const double *B, unsigned k, unsigned P,
               double x, double *rho, double *nx, double *z);

/*Systematic resampling. Writes P values drawn from val[0..n) with
 probabilities w/sum(w), with the offset u in [0, 1). If all weights are 0,
 out stays unchanged and the call returns false.*/
bool rsb2_resample(const double *val, const double *w, unsigned n, double u,
                   double *out, unsigned P);

/*Linear width test of one clause around the corner embedding R2. A[l] and
 B[l] are the SP products of variable l, and h[l] = sum_b w_b/(1-eta_b) over
 its opposite-sign inputs b of width w_b. Same-sign inputs pass no width.
 out[t] = sum_{l!=t} A_l B_l h_l/N_l prod_{l'!=t,l} rho_l'.
 Lean: T_coefficient, coef_code, slope_hasDerivAt.*/
void rsb2_spread(const double *A, const double *B, const double *h,
                 unsigned k, double *out);

/*The same spread with the source of each variable given: out[t] =
 sum_{l!=t} h[l] prod_{l'!=t,l} nu[l']. With h = A B h/N and nu = rho it is
 rsb2_spread.*/
void rsb2_spread_y(const double *h, const double *nu, unsigned k, double *out);

/*Growth rate lambda = log rho(T) per sweep of the widths w <- T w at the
 current SP fixed point. lambda > 0 means that the clusters break into
 sub-clusters (1RSB unstable). -inf if no width can propagate.*/
double rsb2_lambda(Graph &G);

/*The same test at an SP-y fixed point with y = mu1 (rsb2_thermo.cpp). It does
 not depend on mu2 (Lean: charpoly_gauge), and it is rsb2_lambda at y = inf.*/
double rsb2_lambda_y(Graph &G, double y);

/*Population solver. width[0] is the mean width min(eta, 1-eta) after the
 re-seed (none if sweeps = 0), width[t] the mean width after sweep t. phi and
 s = Phi'(x) give Sigma_1 = phi - x s (Lean: hasDerivAt_Phi,
 sigma1_eq_entropy). dmarg is the mean |delta(sT-sF)| between the 2RSB and
 the SP marginals over the variables of the residual formula. spread is
 sum_e Var(eta_e) / sum_e mean(1-mean) over the clusters: 1 at the corners
 R2 and 0 at the atoms R1 (Lean: R2_support, R1_mean), where
 Phi(x) = Sigma_SP and x Sigma_SP.*/
struct Rsb2Obs {
  std::vector<double> width;
  double phi, s, dmarg, spread;
};
Rsb2Obs rsb2_populations(Graph &G, double x, unsigned pop, unsigned sweeps);

/*Hook after Graph::surveys(). Every g_rsb2_every fixed points it appends one
 row to PREFIX_rsb2.csv (rsb2.csv without --diag).*/
void rsb2_step(Graph &G);

extern bool g_rsb2_stab;     /*--rsb2-stab: linear test at each logged step*/
extern double g_rsb2_x;      /*--rsb2-x: x of the population solver, 0 = off*/
extern unsigned g_rsb2_pop;  /*--rsb2-pop: members per edge, default 64*/
extern unsigned g_rsb2_every;/*--rsb2-every: log every K fixed points*/

#endif /* BSP_RSB2_HPP */
