//
//  softq.cpp
//  BSP
//
//  Soft two-step look-ahead for the decimation (paper, Section softq).
//

#include <bsp/softq.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace std;

unsigned g_softq_m = 0;
double g_softq_alpha = 0.;
double g_softq_q = 1.;
bool g_softq_observe = false;
vector<double> g_softq_rollout_at;
const vector<pair<double, double> > g_softq_score_grid = {
    {1., 0.}, {2., 0.}, {4., 0.}, {1., 0.05}, {2., 0.05}};

/*A site frozen against a value has 1-w = 0 and reward -inf. The floor keeps
 the sums finite; its reward is far below any reward of a real move.*/
static const double k_floor = 1e-12;

double softq_reward(double x, double q) {
  x = max(x, k_floor);
  if (q == 1.) return log(x);
  return (pow(x, 1. - q) - 1.) / (1. - q);
}

double softq_site_value(double sT, double sF, double q, double alpha) {
  double rp = softq_reward(1. - sF, q), rm = softq_reward(1. - sT, q);
  double m = max(rp, rm);
  if (alpha <= 0.) return m;
  return m + alpha * log(exp((rp - m) / alpha) + exp((rm - m) / alpha));
}

double softq_log_policy(double sT, double sF, bool dir, double q, double alpha) {
  return softq_reward(dir ? 1. - sF : 1. - sT, q) - softq_site_value(sT, sF, q, alpha);
}

bool softq_parse_rollout(const string& spec) {
  stringstream in(spec);
  string tok;
  while (getline(in, tok, ',')) {
    try {
      double f = stod(tok);
      if (!(f > 0. && f < 1.)) return false;
      g_softq_rollout_at.push_back(f);
    } catch (...) {
      return false;
    }
  }
  sort(g_softq_rollout_at.begin(), g_softq_rollout_at.end());
  return !g_softq_rollout_at.empty();
}
