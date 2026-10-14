#include <bsp/run.hpp>
#include <bsp/Graph.hpp>
#include <bsp/thermo_sp.hpp>
#include <bsp/thermo_policy.hpp>
#include <bsp/tsallis.hpp>
#include <bsp/rsb2.hpp>
#include <bsp/sp2.hpp>
#include <chrono>
#include <cstdarg>
#include <mutex>
#include <thread>
#include <set>
#include <limits>

namespace bsp {
namespace {
RunContext *context = nullptr;
std::mutex run_mutex;
void require(bool condition, const char *message) { if (!condition) throw std::invalid_argument(message); }
// Do not compile this translation unit with fast-math: finite input checks matter.
bool finite(double x) { return std::isfinite(x); }
void apply(const SolverOptions &o) {
    g_scorer_id = -1; g_scorer_gamma = 0.; g_scorer_beta = 1.;
    bsp_parse_scorer(o.scorer);
    g_fixed_seed=o.seed; g_r_bsp=o.r; g_bsp_theta=o.theta; g_epsilon=o.eps; g_damping=o.damping;
    g_T_cav=o.cav_temp; g_T_act=o.act_temp; g_rsb_m=o.rsb_m; g_rsb_gamma=o.rsb_gamma;
    g_gamma_eff=o.rsb_gamma+o.rsb_m*M_LN2; g_tsallis_kappa=o.tsallis_kappa;
    g_tsallis_y=o.cav_temp>0. ? 1./o.cav_temp : 0.;
    g_veto=o.veto; g_minisat_path=o.minisat; g_oracle_timeout=o.oracle_timeout;
    g_oracle_dir=o.oracle_dir; g_oracle_pick=o.oracle_pick; g_lookahead_k=o.lookahead;
    g_corr_batch=o.corr_batch; g_adaptive_r=o.adaptive_r; g_dynamic_i=o.dynamic_i_backtrack;
    g_fe_backtrack=o.fe_backtrack; g_bt_cost=o.bt_cost; g_frac=o.frac;
    g_rsb2_stab=o.rsb2_stab; g_rsb2_x=o.rsb2_x; g_rsb2_pop=o.rsb2_pop; g_rsb2_every=o.rsb2_every?o.rsb2_every:1;
    g_sp2_mu2=o.sp2_mu2; g_sp2_x=o.sp2_x; g_sp2_pop=o.sp2_pop; g_sp2_sweeps=o.sp2_sweeps; g_sp2_every=o.sp2_every;
    g_diag_prefix.clear(); g_diag_every=o.diag_every; g_dump_residuals=false;
    LogLevel level; parse_log_level(o.log_level, level); set_log_level(level); set_log_prefix(o.log_prefix);
}
struct Scope {
    SolverOptions old = current_options();
    std::string diag = g_diag_prefix;
    bool residual = g_dump_residuals, prefix = log_prefix_enabled();
    int scorer_id = g_scorer_id;
    double scorer_gamma = g_scorer_gamma, scorer_beta = g_scorer_beta;
    double effective = g_gamma_eff, y = g_tsallis_y;
    LogLevel level = log_level();
    Scope(RunContext &c, const SolverOptions &o) { apply(o); context=&c; reset_rsb2_populations(); }
    ~Scope() { context=nullptr; reset_rsb2_populations(); apply(old); g_diag_prefix=diag;
        g_dump_residuals=residual; g_scorer_id=scorer_id; g_scorer_gamma=scorer_gamma;
        g_scorer_beta=scorer_beta; g_gamma_eff=effective; g_tsallis_y=y;
        set_log_level(level); set_log_prefix(prefix); }
};
int formatted(bool error, const char *format, va_list args) {
    va_list copy; va_copy(copy,args); int count=vsnprintf(nullptr,0,format,copy); va_end(copy);
    if(count<0) return count;
    std::vector<char> buffer(static_cast<size_t>(count)+1);
    vsnprintf(buffer.data(),buffer.size(),format,args);
    output_stream(error).write(buffer.data(),count);
    if(context) {
        auto &pending=error?context->pending_err:context->pending_out;
        pending.append(buffer.data(),count);
        size_t end;
        while((end=pending.find('\n'))!=std::string::npos) {
            record_log(error?"ERROR":"INFO",error?"stderr":"stdout",pending.substr(0,end+1));
            pending.erase(0,end+1);
        }
    }
    return count;
}
}
RunContext *run_context() { return context; }
void record_populations(Graph &graph,unsigned step,std::vector<std::vector<double>> data) {
    if(!context) return;
    auto &table=context->result->tables["population_edges"];
    table.columns={"step","edge","clause","vertex","sign"};
    unsigned edge=0;
    for(auto *cl:graph._cl_list) for(size_t l=0;l<cl->_go_forward.size();++l) if(cl->_go_forward[l])
        table.rows.push_back({double(step),double(edge++),double(cl->_c+1),double(cl->v_V[l]->_vertex),cl->v_lit[l]?1.:-1.});
    context->result->populations.emplace(step,std::move(data));
}
void check_cancelled() { if(context && context->cancelled && context->cancelled()) throw Cancelled(); }
std::ostream &output_stream(bool error) { return context ? (error ? static_cast<std::ostream&>(context->err) : context->out) : (error?std::cerr:std::cout); }
void record_log(const std::string &level,const std::string &channel,const std::string &message) {
    if(context && !message.empty()) context->result->logs.push_back({level,channel,message});
}
int console_printf(const char *format, ...) { va_list args; va_start(args,format); int n=formatted(false,format,args); va_end(args); return n; }
int console_fprintf(FILE *stream,const char *format,...) {
    va_list args; va_start(args,format);
    int n = (stream==stdout || stream==stderr) ? formatted(stream==stderr,format,args) : vfprintf(stream,format,args);
    va_end(args); return n;
}
void Formula::validate() const {
    require(n>0 && n<=1000000,"n must be in [1, 1000000] (WalkSAT limit)");
    require(clauses.size()<=std::numeric_limits<unsigned>::max(),"too many clauses");
    for(const auto &cl:clauses) {
        require(cl.size()<=500,"clauses may contain at most 500 literals (WalkSAT limit)");
        std::set<unsigned> seen;
        for(int lit:cl) {
            require(lit!=0 && lit!=std::numeric_limits<int>::min(),"literals must be nonzero signed integers");
            unsigned var=static_cast<unsigned>(std::abs(lit));
            require(var<=n,"literal exceeds n");
            require(seen.insert(var).second,"a variable may occur only once in each clause");
        }
    }
}
void SolverOptions::validate() const {
    for(double x:{r,theta,eps,damping,cav_temp,act_temp,rsb_m,rsb_gamma,tsallis_kappa,bt_cost,frac,rsb2_x,sp2_mu2,sp2_x})
        require(finite(x),"solver parameters must be finite");
    require(seed==-1 || (seed>=1 && static_cast<unsigned long>(seed)<=std::numeric_limits<unsigned>::max()),"seed must be None or in [1, UINT_MAX]");
    require(r>=0. && r<1.,"r must be in [0, 1)"); require(theta>=0. && theta<=1.,"theta must be in [0, 1]");
    require(eps>0.,"eps must be > 0"); require(damping>=0. && damping<1.,"damping must be in [0, 1)");
    require(cav_temp>=0. && act_temp>=0.,"temperatures must be nonnegative");
    require(frac>0. && frac<=1.,"frac must be in (0, 1]"); require(diag_every>0,"diag_every must be >= 1");
    require(rsb2_x>=0. && rsb2_x<=1.,"rsb2_x must be in [0, 1]");
    require(sp2_mu2>=0. && sp2_x>0. && sp2_x<=1.,"sp2_mu2 must be nonnegative and sp2_x in (0, 1]");
    require(rsb2_pop>=2 && sp2_pop>=2,"population sizes must be >= 2");
    require(!(sp2_mu2>0. && rsb2_x>0.),"sp2_mu2 does not combine with rsb2_x");
    require(oracle_pick==0,"oracle_pick is not implemented (use oracle_dir)");
    require(!(act_temp>0. || fe_backtrack || tsallis_kappa!=0. || scorer=="fth") || cav_temp>0.,"fth, act_temp, fe_backtrack and tsallis_kappa require cav_temp > 0");
    require(!(rsb2_stab || rsb2_x>0.) || rsb_gamma+rsb_m*M_LN2==0.,"rsb2 tools require SP or SP-y without an unfrozen tilt");
    require(!(rsb2_x>0. && cav_temp>0.),"rsb2_x requires plain SP");
    LogLevel level; require(parse_log_level(log_level,level),"unknown log_level");
    // Parse parameterized scorers without changing global solver state.
    if(scorer=="cert" || scorer=="pol" || scorer=="i_c" || scorer=="fth" || scorer=="rsb") return;
    size_t colon=scorer.find(':'); require(colon!=std::string::npos,"unknown scorer");
    std::string name=scorer.substr(0,colon); size_t used=0; double value;
    try { value=std::stod(scorer.substr(colon+1),&used); }
    catch(const std::exception &) { throw std::invalid_argument("invalid scorer parameter"); }
    require(used==scorer.size()-colon-1 && finite(value),"invalid scorer parameter");
    require((name=="gamma" && value>=0.) || (name=="soft_cert" && value>0.),"unknown scorer or invalid scorer parameter");
}
SolverOptions current_options() {
    SolverOptions o;
    o.scorer=bsp_scorer_name(); o.seed=g_fixed_seed; o.r=g_r_bsp; o.theta=g_bsp_theta; o.eps=g_epsilon; o.damping=g_damping;
    o.cav_temp=g_T_cav; o.act_temp=g_T_act; o.rsb_m=g_rsb_m; o.rsb_gamma=g_rsb_gamma; o.tsallis_kappa=g_tsallis_kappa;
    o.veto=g_veto; o.minisat=g_minisat_path; o.oracle_timeout=g_oracle_timeout; o.oracle_dir=g_oracle_dir;
    o.oracle_pick=g_oracle_pick; o.lookahead=g_lookahead_k; o.corr_batch=g_corr_batch; o.adaptive_r=g_adaptive_r;
    o.dynamic_i_backtrack=g_dynamic_i; o.fe_backtrack=g_fe_backtrack; o.bt_cost=g_bt_cost; o.frac=g_frac;
    o.rsb2_stab=g_rsb2_stab; o.rsb2_x=g_rsb2_x; o.rsb2_pop=g_rsb2_pop; o.rsb2_every=g_rsb2_every;
    o.sp2_mu2=g_sp2_mu2; o.sp2_x=g_sp2_x; o.sp2_pop=g_sp2_pop; o.sp2_sweeps=g_sp2_sweeps; o.sp2_every=g_sp2_every;
    o.diag_every=g_diag_every; return o;
}
Formula generate_formula(unsigned k,double alpha,unsigned n,long seed) {
    require(n>0 && n<=1000000 && k>=1 && k<=n && k<=500,"require 1 <= k <= min(n, 500), n <= 1000000");
    require(finite(alpha) && alpha>=0. && alpha*n<=std::numeric_limits<unsigned>::max(),"invalid alpha");
    SolverOptions o; o.seed=seed; o.validate();
    std::lock_guard<std::mutex> lock(run_mutex);
    SolveResult result; RunContext ctx{&result,&o,{}}; Scope scope(ctx,o);
    std::vector<std::string> args={"bsp","-w",std::to_string(k),([&]() { std::ostringstream s; s << std::setprecision(17) << alpha; return s.str(); })(),std::to_string(n)};
    std::vector<char*> argv; for(auto &a:args) argv.push_back(&a[0]);
    Graph graph(argv.size(),argv.data()); graph.write_on_file_graph();
    Formula f=graph.formula(); f.random_k=k; f.random_alpha=alpha; f.validate(); return f;
}
SolveResult solve(const Formula &formula,const SolverOptions &options,std::function<bool()> cancelled) {
    formula.validate(); options.validate();
    std::unique_lock<std::mutex> lock(run_mutex,std::defer_lock);
    while(!lock.try_lock()) {
        if(cancelled && cancelled()) { SolveResult r; r.formula=formula; r.status="cancelled"; r.reason="cancelled while waiting for solver"; return r; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    SolverOptions o=options; if(o.seed<0 && formula.seed>=1) o.seed=formula.seed;
    SolveResult result; result.formula=formula;
    RunContext ctx{&result,&o,std::move(cancelled)}; Scope scope(ctx,o);
    auto start=std::chrono::steady_clock::now();
    std::vector<Vertex> vertices;
    const char *arg="bsp"; char *argv[]={const_cast<char*>(arg)};
    Graph graph(1,argv); result.seed=graph.seed();
    try {
        check_cancelled();
        BSP_INFO<<"START ALGORITHM:"<<endl;
        // Replay generation to preserve the CLI's RNG position for random inputs.
        if(formula.random_k && result.seed==formula.seed) graph.consume_generation(formula.random_k,formula.random_alpha);
        graph.load_formula(formula);
        if(formula.clauses.empty()) {
            result.solutions.push_back(std::vector<int>(formula.n));
            for(unsigned i=0;i<formula.n;++i) result.solutions.back()[i]=-static_cast<int>(i+1);
            result.status="solved";
        } else {
            for(const auto &cl:formula.clauses) if(cl.empty()) throw HeuristicFailure("input contains an empty clause");
            if(run_algorithm(graph,vertices)) result.status="solved";
            else result.reason="WalkSAT did not find a solution";
        }
    } catch(const Cancelled &e) { result.status="cancelled"; result.reason=e.what(); }
      catch(const HeuristicFailure &e) { result.status="failed"; result.reason=e.what(); }
      catch(const std::runtime_error &e) { result.status="error"; result.reason=e.what(); }
    if(!vertices.empty()) for(const auto &v:vertices) {
        result.surveys.push_back(v._sT); result.surveys.push_back(v._sF); result.surveys.push_back(v._sI);
    }
    record_log("INFO","stdout",ctx.pending_out); record_log("ERROR","stderr",ctx.pending_err);
    result.stdout_text=ctx.out.str(); result.stderr_text=ctx.err.str();
    result.wall_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    return result;
}
}
