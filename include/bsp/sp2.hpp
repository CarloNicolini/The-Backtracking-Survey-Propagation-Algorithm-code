//
//  sp2.hpp
//  BSP
//
//  Finite-mu 2RSB survey propagation on one formula (paper eq. sp2).
//  Each active edge carries a population of surveys. One sweep replaces
//  every survey by the product of pi/z over the other variables of its
//  clause. z and pi are the energetic averages at mu2, and a candidate is
//  kept with probability (z / z_max)^x, where z_max is z at the smallest
//  atom of every incoming edge. This is not --rsb2-x.
//

#ifndef BSP_SP2_HPP
#define BSP_SP2_HPP

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

class Graph;

/*z and nu = pi/z at pseudo-temperature 1/mu. es and eu are one survey per
 incoming edge, same sign and opposite sign. pi is the weight of q > p.*/
void sp2_nu(const double *es, std::size_t ns, const double *eu, std::size_t nu,
            double mu, double &z, double &v);

/*M samples of nu from the law reweighted by z^x. S[b] and U[b] point at M0
 atoms. drawn counts the candidates. Returns false when a candidate beats
 the bound or the budget ends before M accepts.*/
bool sp2_variable(const std::vector<const double *> &S, const std::vector<const double *> &U,
                  std::size_t M0, double mu, double x, unsigned M, unsigned budget,
                  std::mt19937_64 &rng, unsigned &drawn, double *out);

struct Sp2Obs {
    double mean, spread, diff, drawn_per_keep;
    unsigned trunc;
};

/*sweeps of eq. sp2 from the corner embedding of the current surveys.*/
Sp2Obs sp2_run(Graph &G, double mu2, double x, unsigned pop, unsigned sweeps);

/*--sp2-mu2 > 0 runs sp2_run at the first SP fixed point, then every
 --sp2-every fixed points. --sp2-every 0 runs once.*/
void sp2_step(Graph &G);

extern double g_sp2_mu2;       /*--sp2-mu2, 0 = off*/
extern double g_sp2_x;         /*--sp2-x, default 0.5*/
extern unsigned g_sp2_pop;     /*--sp2-pop, default 32*/
extern unsigned g_sp2_sweeps;  /*--sp2-sweeps, default 4*/
extern unsigned g_sp2_every;   /*--sp2-every, 0 = once*/

#endif /* BSP_SP2_HPP */
