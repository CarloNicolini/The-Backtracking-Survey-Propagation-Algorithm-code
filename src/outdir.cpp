//
// outdir.cpp — isolate bsp file outputs under --outdir (or bsp_runs/<slug>).
//

#include <bsp/outdir.hpp>

#include <bsp/Header.hpp>
#include <bsp/thermo_policy.hpp>
#include <bsp/thermo_sp.hpp>
#include <bsp/tsallis.hpp>

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

using namespace std;

namespace {

string json_escape(const string& s) {
  string o;
  o.reserve(s.size() + 8);
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '\\' || c == '"') {
      o.push_back('\\');
      o.push_back(c);
    } else if (c == '\n') {
      o += "\\n";
    } else if (c == '\r') {
      o += "\\r";
    } else if (c == '\t') {
      o += "\\t";
    } else {
      o.push_back(c);
    }
  }
  return o;
}

string float_token(double x) {
  ostringstream o;
  o.setf(ios::fmtflags(0), ios::floatfield);
  o << x;
  string s = o.str();
  for (size_t i = 0; i < s.size(); ++i)
    if (s[i] == '.') s[i] = 'p';
  return s;
}

int mkdir_p(const string& path) {
  if (path.empty() || path == ".") return 0;
  string cur;
  for (size_t i = 0; i < path.size(); ++i) {
    char c = path[i];
    cur.push_back(c);
    if (c == '/' || i + 1 == path.size()) {
      if (cur == "/" || cur == "./" || cur == ".") continue;
      string dir = (c == '/' && i + 1 != path.size()) ? cur.substr(0, cur.size() - 1) : cur;
      if (dir.empty() || dir == ".") continue;
      if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) return -1;
    }
  }
  return 0;
}

string abs_path(const string& path) {
  if (path.empty()) return path;
  if (path[0] == '/') return path;
  char* rp = realpath(path.c_str(), NULL);
  if (rp) {
    string out(rp);
    free(rp);
    return out;
  }
  char cwd[4096];
  if (!getcwd(cwd, sizeof cwd)) return path;
  if (path.compare(0, 2, "./") == 0) return string(cwd) + "/" + path.substr(2);
  return string(cwd) + "/" + path;
}

} // namespace

string bsp_make_run_slug(unsigned K, double alpha, unsigned N) {
  ostringstream o;
  if (K == 0 && N == 0)
    o << "load";
  else
    o << "K" << K << "_N" << N << "_a" << float_token(alpha);
  if (g_fixed_seed >= 1) o << "_s" << g_fixed_seed;
  o << "_" << bsp_scorer_name();
  o << "_r" << float_token(g_r_bsp);
  if (g_T_cav > 0.) o << "_Tc" << float_token(g_T_cav);
  if (g_T_act > 0.) o << "_Ta" << float_token(g_T_act);
  if (g_rsb_m != 0.) o << "_m" << float_token(g_rsb_m);
  if (g_rsb_gamma != 0.) o << "_g" << float_token(g_rsb_gamma);
  if (g_tsallis_kappa != 0.) o << "_kq" << float_token(g_tsallis_kappa);
  if (g_dynamic_i) o << "_dynI";
  if (g_fe_backtrack) o << "_feBT";
  if (g_fe_backtrack) o << "_btc" << float_token(g_bt_cost);
  if (g_frac != frac) o << "_f" << float_token(g_frac);
  if (g_lookahead_k > 0) o << "_la" << g_lookahead_k;
  if (g_adaptive_r) o << "_ar";
  if (g_corr_batch) o << "_cb";
  return o.str();
}

void bsp_write_manifest(const string& path, const BspRunManifest& m) {
  ofstream out(path.c_str());
  if (!out) {
    BSP_ERROR << "Cannot write manifest " << path << endl;
    return;
  }
  out << "{\n";
  out << "  \"outdir\": \"" << json_escape(m.outdir) << "\",\n";
  out << "  \"cwd_before\": \"" << json_escape(m.cwd_before) << "\",\n";
  out << "  \"mode\": \"" << json_escape(m.mode) << "\",\n";
  out << "  \"load\": "
      << (m.load_path.empty() ? "null" : ("\"" + json_escape(m.load_path) + "\""))
      << ",\n";
  out << "  \"K\": " << m.K << ",\n";
  out << "  \"alpha\": " << m.alpha << ",\n";
  out << "  \"N\": " << m.N << ",\n";
  out << "  \"seed\": " << m.seed << ",\n";
  out << "  \"scorer\": \"" << json_escape(m.scorer) << "\",\n";
  out << "  \"r\": " << m.r << ",\n";
  out << "  \"cav_temp\": " << m.cav_temp << ",\n";
  out << "  \"act_temp\": " << m.act_temp << ",\n";
  out << "  \"rsb_m\": " << m.rsb_m << ",\n";
  out << "  \"rsb_gamma\": " << m.rsb_gamma << ",\n";
  out << "  \"tsallis_kappa\": " << m.tsallis_kappa << ",\n";
  out << "  \"dynamic_i\": " << (m.dynamic_i ? "true" : "false") << ",\n";
  out << "  \"fe_backtrack\": " << (m.fe_backtrack ? "true" : "false") << ",\n";
  out << "  \"bt_cost\": " << m.bt_cost << ",\n";
  out << "  \"frac\": " << m.frac_batch << ",\n";
  out << "  \"eps\": " << m.eps << ",\n";
  out << "  \"damping\": " << m.damping << ",\n";
  out << "  \"theta\": " << m.theta << ",\n";
  out << "  \"diag\": "
      << (m.diag.empty() ? "null" : ("\"" + json_escape(m.diag) + "\""))
      << ",\n";
  out << "  \"diag_every\": " << m.diag_every << ",\n";
  out << "  \"rsb2_stab\": " << (m.rsb2_stab ? "true" : "false") << ",\n";
  out << "  \"rsb2_x\": " << m.rsb2_x << ",\n";
  out << "  \"rsb2_pop\": " << m.rsb2_pop << ",\n";
  out << "  \"rsb2_every\": " << m.rsb2_every << ",\n";
  out << "  \"timestamp_utc\": \"" << json_escape(m.timestamp_utc) << "\",\n";
  out << "  \"argv\": [";
  for (size_t i = 0; i < m.argv.size(); ++i) {
    if (i) out << ", ";
    out << "\"" << json_escape(m.argv[i]) << "\"";
  }
  out << "]\n";
  out << "}\n";
}

int bsp_enter_outdir(const string& outdir_arg, bool have_outdir_flag, unsigned K,
                     double alpha, unsigned N, const string& load_path_in,
                     string& load_path_out, string& outdir_abs,
                     string& cwd_before) {
  char cwd[4096];
  if (!getcwd(cwd, sizeof cwd)) {
    BSP_ERROR << "getcwd failed: " << strerror(errno) << endl;
    return -1;
  }
  cwd_before = cwd;

  string target;
  if (have_outdir_flag) {
    target = outdir_arg.empty() ? "." : outdir_arg;
  } else {
    target = string("bsp_runs/") + bsp_make_run_slug(K, alpha, N);
  }

  if (!load_path_in.empty())
    load_path_out = abs_path(load_path_in);
  else
    load_path_out.clear();

  if (g_diag_prefix.size() > 0 && g_diag_prefix[0] != '/') {
    /* keep relative diag prefix; it will land inside outdir after chdir */
  }

  if (mkdir_p(target) != 0) {
    BSP_ERROR << "Cannot create outdir " << target << ": " << strerror(errno)
              << endl;
    return -1;
  }

  outdir_abs = abs_path(target);
  if (chdir(outdir_abs.c_str()) != 0) {
    BSP_ERROR << "Cannot chdir to " << outdir_abs << ": " << strerror(errno)
              << endl;
    return -1;
  }

  BSP_INFO << "Output directory: " << outdir_abs << endl;
  return 0;
}

string bsp_utc_now() {
  time_t t = time(NULL);
  struct tm tm_buf;
  gmtime_r(&t, &tm_buf);
  char buf[64];
  strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
  return string(buf);
}
