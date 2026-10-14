#ifndef BSP_RUN_HPP
#define BSP_RUN_HPP
#include <cstdio>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
class Graph;
class Vertex;
namespace bsp {
struct Cell {
    double number = 0.;
    std::string text;
    bool is_text = false;
    bool is_missing = false;
    Cell() : is_missing(true) {}
    Cell(double value) : number(value) {}
    Cell(const char *value) : text(value), is_text(true) {}
};
struct Table {
    std::vector<std::string> columns;
    std::vector<std::vector<Cell>> rows;
};
struct LogRecord { std::string level, channel, message; };
struct Formula {
    unsigned n = 0;
    std::vector<std::vector<int>> clauses;
    long seed = -1;
    unsigned random_k = 0;
    double random_alpha = 0.;
    void validate() const;
};
struct SolverOptions {
    std::string scorer = "cert", minisat = "minisat", log_level = "info";
    long seed = -1;
    double r = .9, theta = 0., eps = .01, damping = 0., cav_temp = 0., act_temp = 0.;
    double rsb_m = 0., rsb_gamma = 0., tsallis_kappa = 0., bt_cost = .4, frac = .00125;
    bool veto = false, oracle_dir = false, corr_batch = false, adaptive_r = false;
    bool dynamic_i_backtrack = false, fe_backtrack = false, rsb2_stab = false;
    unsigned oracle_timeout = 10, oracle_pick = 0, lookahead = 0;
    double rsb2_x = 0., sp2_mu2 = 0., sp2_x = .5;
    unsigned rsb2_pop = 64, rsb2_every = 1, sp2_pop = 32, sp2_sweeps = 4, sp2_every = 0;
    bool variable_history = false, residual_history = false, population_history = false;
    unsigned diag_every = 1;
    bool log_prefix = false;
    void validate() const;
};
struct SolveResult {
    std::string status = "failed", reason, stdout_text, stderr_text;
    Formula formula;
    long seed = -1;
    double wall_seconds = 0.;
    std::vector<std::vector<int>> solutions; // signed literals in variable order
    std::vector<double> surveys; // N x 3, columns sT, sF, sI
    std::map<std::string, Table> tables;
    std::map<unsigned, Formula> residuals;
    std::map<unsigned, std::vector<std::vector<double>>> populations;
    std::vector<LogRecord> logs;
};
struct HeuristicFailure : std::runtime_error { using std::runtime_error::runtime_error; };
struct Cancelled : std::runtime_error { Cancelled() : std::runtime_error("cancelled") {} };
struct RunContext {
    SolveResult *result;
    const SolverOptions *options;
    std::function<bool()> cancelled;
    std::ostringstream out, err;
    std::string pending_out, pending_err;
    unsigned rsb2_step = 0, sp2_step = 0;
};
RunContext *run_context();
void check_cancelled();
void record_log(const std::string &level, const std::string &channel, const std::string &message);
std::ostream &output_stream(bool error);
int console_printf(const char *format, ...);
int console_fprintf(FILE *stream, const char *format, ...);
SolverOptions current_options();
Formula generate_formula(unsigned k, double alpha, unsigned n, long seed);
SolveResult solve(const Formula &, const SolverOptions &, std::function<bool()> cancelled = {});
bool run_algorithm(Graph &, std::vector<Vertex> &);
void reset_rsb2_populations();
void record_populations(Graph &, unsigned step, std::vector<std::vector<double>> data);
}
#endif
