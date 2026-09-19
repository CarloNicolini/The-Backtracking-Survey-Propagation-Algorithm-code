#ifndef NNSCORER_HPP
#define NNSCORER_HPP

#include <string>

/*No-op stubs so the C++ oracle builds without GenANN / bsp-train.
 Python SAC work does not use this path.*/

inline bool bsp_nn_load(const std::string&) { return false; }
inline bool bsp_nn_ready() { return false; }
inline bool bsp_nn_predict(const double*, double& y) {
    y = 0.0 / 0.0;
    return false;
}

#endif
