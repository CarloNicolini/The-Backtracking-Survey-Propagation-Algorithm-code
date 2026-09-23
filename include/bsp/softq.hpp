//
//  softq.hpp
//  BSP
//
//  Soft two-step look-ahead for the decimation (paper, Section softq).
//

#ifndef SOFTQ_HPP
#define SOFTQ_HPP

#include <string>
#include <vector>

/*Number M of candidate variables probed at each decimation. The candidates
 are the top-M free variables in the BSP order. M <= batch keeps plain BSP.*/
extern unsigned g_softq_m;
/*Policy temperature alpha of the soft Bellman value. alpha=0 is the hard limit.*/
extern double g_softq_alpha;
/*Tsallis index q of the reward ln_q(1-w). q=1 is the logarithm.*/
extern double g_softq_q;
/*Score and log the candidates, but keep the BSP order.*/
extern bool g_softq_observe;
/*Fractions of fixed variables at which each candidate is rolled out with plain BSP.*/
extern std::vector<double> g_softq_rollout_at;
/*(q, alpha) pairs whose scores are also logged at a rollout checkpoint.*/
extern const std::vector<std::pair<double, double> > g_softq_score_grid;

/*Reward ln_q(x) of a move that keeps the fraction x = 1-w of the clusters.*/
double softq_reward(double x, double q);

/*Soft value v_alpha of one site: alpha log(e^{r+/alpha} + e^{r-/alpha}),
 with r+ = ln_q(1-sF) and r- = ln_q(1-sT). At alpha=0 it is max(r+, r-).*/
double softq_site_value(double sT, double sF, double q, double alpha);

/*alpha log pi_alpha(dir) = r^dir - v_alpha, the SAC identity for the sign.*/
double softq_log_policy(double sT, double sF, bool dir, double q, double alpha);

/*Parse "0.2,0.4" into g_softq_rollout_at. Returns false on a bad token.*/
bool softq_parse_rollout(const std::string& spec);

#endif /* SOFTQ_HPP */
