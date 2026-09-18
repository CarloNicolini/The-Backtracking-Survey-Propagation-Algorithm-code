#include "NnScorer.hpp"
#include "third_party/genann/genann.h"
#include <cmath>

static genann* g_ann = 0;
static vector<double> g_mean;
static vector<double> g_stdv;
static bool g_ready = false;

bool bsp_nn_load(const string& path) {
    bsp_nn_unload();
    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        BSP_ERROR << "Cannot open NN weights file " << path << endl;
        return false;
    }
    g_mean.assign(BSP_NN_NFEAT, 0.0);
    g_stdv.assign(BSP_NN_NFEAT, 1.0);
    char line[4096];
    /*Header lines start with '#'. mean/std rows: "# mean v0 v1 ..." */
    long pos = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] != '#') {
            fseek(f, pos, SEEK_SET);
            break;
        }
        if (strncmp(line, "# mean", 6) == 0) {
            char* p = line + 6;
            for (int i = 0; i < BSP_NN_NFEAT; ++i)
                g_mean[static_cast<size_t>(i)] = strtod(p, &p);
        } else if (strncmp(line, "# std", 5) == 0) {
            char* p = line + 5;
            for (int i = 0; i < BSP_NN_NFEAT; ++i) {
                double s = strtod(p, &p);
                g_stdv[static_cast<size_t>(i)] = (s > 1e-12) ? s : 1.0;
            }
        }
        pos = ftell(f);
    }
    g_ann = genann_read(f);
    fclose(f);
    if (!g_ann) {
        BSP_ERROR << "genann_read failed for " << path << endl;
        return false;
    }
    if (g_ann->inputs != BSP_NN_NFEAT || g_ann->outputs != 1) {
        BSP_ERROR << "NN shape mismatch: got inputs=" << g_ann->inputs
                  << " outputs=" << g_ann->outputs
                  << " expected " << BSP_NN_NFEAT << "x1" << endl;
        bsp_nn_unload();
        return false;
    }
    g_ann->activation_output = genann_act_linear;
    g_ready = true;
    BSP_INFO << "Loaded NN weights from " << path
             << " (hidden=" << g_ann->hidden << ")" << endl;
    return true;
}

void bsp_nn_unload() {
    if (g_ann) genann_free(g_ann);
    g_ann = 0;
    g_ready = false;
}

bool bsp_nn_ready() {
    return g_ready;
}

bool bsp_nn_predict(const double* x, double& y) {
    if (!g_ready) return false;
    double z[BSP_NN_NFEAT];
    for (int i = 0; i < BSP_NN_NFEAT; ++i) {
        z[i] = (x[i] - g_mean[static_cast<size_t>(i)]) / g_stdv[static_cast<size_t>(i)];
        if (fabs(z[i]) > 8.0) return false;
    }
    y = genann_run(g_ann, z)[0];
    return true;
}
