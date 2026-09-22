#ifndef NnScorer_hpp
#define NnScorer_hpp

#include "Header.h"

/*Tiny GenANN wrapper: load a weights file written by bsp-train, score a
 local feature vector. No-op (bsp_nn_ready()==false) when no file is loaded.*/

bool bsp_nn_load(const string &path);
void bsp_nn_unload();
bool bsp_nn_ready();
/*Standardize x[0..BSP_NN_NFEAT) with the file's mean/std and set y to the
 predicted DeltaSigma. Returns false (OOD abstention: caller keeps the
 hand-crafted score) when not loaded or any |z-score| exceeds 8. Bool,
 not NaN: NaN comparisons are unreliable under -ffast-math.*/
bool bsp_nn_predict(const double *x, double &y);

#endif
