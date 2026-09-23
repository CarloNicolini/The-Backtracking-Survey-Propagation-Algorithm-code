//
// outdir.hpp — run isolation and manifest for bsp file outputs.
//

#ifndef BSP_OUTDIR_HPP
#define BSP_OUTDIR_HPP

#include <string>
#include <vector>

/*Build a filesystem-safe slug from the active CLI theory parameters.*/
std::string bsp_make_run_slug(unsigned K, double alpha, unsigned N);

/*Create outdir (or bsp_runs/<slug> when have_outdir_flag is false), resolve
 load_path to absolute, chdir into outdir. Returns 0 on success.*/
int bsp_enter_outdir(const std::string& outdir_arg, bool have_outdir_flag,
                     unsigned K, double alpha, unsigned N,
                     const std::string& load_path_in, std::string& load_path_out,
                     std::string& outdir_abs, std::string& cwd_before);

/*Write manifest.json describing the run so the CLI can be reconstructed.*/
struct BspRunManifest {
  std::string outdir;
  std::string cwd_before;
  std::string mode; /*"write" or "load"*/
  std::string load_path;
  unsigned K;
  double alpha;
  unsigned N;
  long seed;
  std::string scorer;
  double r;
  double cav_temp;
  double act_temp;
  double rsb_m;
  double rsb_gamma;
  bool dynamic_i;
  bool fe_backtrack;
  double bt_cost;
  double frac_batch;
  double eps;
  double damping;
  double theta;
  std::string diag;
  unsigned diag_every;
  std::string timestamp_utc;
  std::vector<std::string> argv;
};

void bsp_write_manifest(const std::string& path, const BspRunManifest& m);

std::string bsp_utc_now();

#endif /* BSP_OUTDIR_HPP */
