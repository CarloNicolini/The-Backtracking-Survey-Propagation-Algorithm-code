#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>
#include <bsp/run.hpp>
#include <atomic>
#include <limits>
namespace nb = nanobind;
namespace {
struct CancellationToken { std::atomic<bool> requested{false}; };
template<class T> nb::ndarray<nb::numpy,T> array(std::vector<T> data, std::initializer_list<size_t> shape) {
    auto *storage=new std::vector<T>(std::move(data));
    nb::capsule owner(storage, [](void *p) noexcept { delete static_cast<std::vector<T>*>(p); });
    return nb::ndarray<nb::numpy,T>(storage->data(),shape,owner);
}
nb::dict tables(const bsp::SolveResult &result) {
    nb::dict out;
    for(const auto &entry:result.tables) {
        nb::dict table; table["columns"]=nb::cast(entry.second.columns);
        nb::list rows;
        for(const auto &row:entry.second.rows) {
            nb::list values;
            for(const auto &cell:row) { if(cell.is_missing) values.append(nb::float_(std::numeric_limits<double>::quiet_NaN())); else if(cell.is_text) values.append(nb::str(cell.text.c_str())); else values.append(nb::float_(cell.number)); }
            rows.append(values);
        }
        table["rows"]=rows; out[nb::str(entry.first.c_str())]=table;
    }
    return out;
}
}
NB_MODULE(_native,m) {
    m.doc()="Native in-memory BSP API linked against bsp_core";
    nb::class_<CancellationToken>(m,"CancellationToken")
        .def(nb::init<>()).def("cancel",[](CancellationToken &t) { t.requested.store(true); })
        .def_prop_ro("requested",[](CancellationToken &t) { return t.requested.load(); });
    nb::class_<bsp::Formula>(m,"Formula").def(nb::init<>())
        .def_rw("n",&bsp::Formula::n).def_rw("clauses",&bsp::Formula::clauses)
        .def_rw("seed",&bsp::Formula::seed).def_rw("random_k",&bsp::Formula::random_k)
        .def_rw("random_alpha",&bsp::Formula::random_alpha).def("validate",&bsp::Formula::validate);
    auto options=nb::class_<bsp::SolverOptions>(m,"SolverOptions").def(nb::init<>()).def("validate",&bsp::SolverOptions::validate);
#define OPTION(name) options.def_rw(#name,&bsp::SolverOptions::name)
    OPTION(scorer); OPTION(minisat); OPTION(log_level); OPTION(seed); OPTION(r); OPTION(theta); OPTION(eps); OPTION(damping);
    OPTION(cav_temp); OPTION(act_temp); OPTION(rsb_m); OPTION(rsb_gamma); OPTION(tsallis_kappa); OPTION(bt_cost); OPTION(frac);
    OPTION(veto); OPTION(oracle_dir); OPTION(corr_batch); OPTION(adaptive_r); OPTION(dynamic_i_backtrack); OPTION(fe_backtrack);
    OPTION(rsb2_stab); OPTION(oracle_timeout); OPTION(oracle_pick); OPTION(lookahead); OPTION(rsb2_x); OPTION(sp2_mu2); OPTION(sp2_x);
    OPTION(rsb2_pop); OPTION(rsb2_every); OPTION(sp2_pop); OPTION(sp2_sweeps); OPTION(sp2_every);
    OPTION(variable_history); OPTION(residual_history); OPTION(population_history); OPTION(diag_every); OPTION(log_prefix);
#undef OPTION
    nb::class_<bsp::SolveResult>(m,"SolveResult")
        .def_ro("status",&bsp::SolveResult::status).def_ro("reason",&bsp::SolveResult::reason)
        .def_ro("stdout",&bsp::SolveResult::stdout_text).def_ro("stderr",&bsp::SolveResult::stderr_text)
        .def_ro("seed",&bsp::SolveResult::seed).def_ro("wall_seconds",&bsp::SolveResult::wall_seconds)
        .def_ro("formula",&bsp::SolveResult::formula).def_prop_ro("tables",&tables)
        .def_prop_ro("solutions",[](const bsp::SolveResult &r) {
            std::vector<int> data; for(const auto &s:r.solutions) data.insert(data.end(),s.begin(),s.end());
            return array(std::move(data),{r.solutions.size(),r.formula.n});
        }, nb::rv_policy::move)
        .def_prop_ro("surveys",[](const bsp::SolveResult &r) { return array(r.surveys,{r.surveys.size()/3,3}); }, nb::rv_policy::move)
        .def_ro("residuals",&bsp::SolveResult::residuals)
        .def_prop_ro("populations",[](const bsp::SolveResult &r) {
            nb::dict out;
            for(const auto &item:r.populations) {
                std::vector<double> data; for(const auto &row:item.second) data.insert(data.end(),row.begin(),row.end());
                size_t width=item.second.empty()?0:item.second.front().size();
                out[nb::int_(item.first)]=array(std::move(data),{item.second.size(),width}).cast(nb::rv_policy::move);
            }
            return out;
        })
        .def_prop_ro("logs",[](const bsp::SolveResult &r) {
            nb::list out;
            for(const auto &record:r.logs) { nb::dict d; d["level"]=nb::cast(record.level); d["channel"]=nb::cast(record.channel); d["message"]=nb::cast(record.message); out.append(d); }
            return out;
        });
    m.def("generate_formula",&bsp::generate_formula,nb::arg("k"),nb::arg("alpha"),nb::arg("n"),nb::arg("seed")=-1,nb::call_guard<nb::gil_scoped_release>());
    m.def("solve",[](const bsp::Formula &f,const bsp::SolverOptions &o,CancellationToken *token) {
        // Copy inputs while holding the GIL; no Python object is touched by core computation.
        bsp::Formula formula=f; bsp::SolverOptions options=o;
        nb::gil_scoped_release release;
        return bsp::solve(formula,options,[token]() {
            if(token && token->requested.load()) return true;
            nb::gil_scoped_acquire acquire;
            if(PyErr_CheckSignals()<0) {
                if(PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) { PyErr_Clear(); return true; }
                throw nb::python_error();
            }
            return false;
        });
    },nb::arg("formula"),nb::arg("options"),nb::arg("cancel")=nb::none());
}
