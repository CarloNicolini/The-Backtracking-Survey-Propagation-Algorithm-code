# The-Backtracking-Survey-Propagation-Algorithm-code

This code solves random K-SAT instances using Survey Inspired Decimation (SID)
or Backtracking Survey Propagation (BSP).

If you want a faster version of this code, email: raffaele.marino@unifi.it
or marinoraffaele.nunziatella@gmail.com

For any problem, email: marinoraffaele.nunziatella@gmail.com

## Build (CMake)

Requires CMake ≥ 3.16 and a C++11 compiler (g++ on Linux, AppleClang/Clang on
macOS, or MSVC on Windows).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The executable is `build/main`.

### Options

| CMake option | Default | Meaning |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Release` | `Debug`, `Release`, `RelWithDebInfo`, or `MinSizeRel` |
| `BSP_NATIVE_ARCH` | `ON` | Optimize for the host CPU (`-march=native` / MSVC `/arch:AVX2` on x64). Turn **off** for portable binaries. |

Examples:

```bash
# Portable Release binary
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBSP_NATIVE_ARCH=OFF

# Debug build (symbols; useful for Valgrind)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBSP_NATIVE_ARCH=OFF
```

Release builds enable strong optimization flags per toolchain (MSVC, AppleClang,
Linux g++), including link-time optimization (IPO/LTO) when supported.

Optional install:

```bash
cmake --install build --prefix /path/to/prefix
```

## Usage

Generate a random instance and solve it:

```bash
./build/main -w <K> <alpha> <N>
```

Load a CNF file and solve it:

```bash
./build/main -l <formula.cnf>
```

Example (3-SAT, clause density 4.0, 50 variables):

```bash
./build/main -w 3 4.0 50
```

## Thermodynamic Survey Propagation (ThermoSP)

The option `--cav-temp=T` deforms the SP cavity aggregation with a temperature
T. The default T = 0 keeps the hard SP factors and the current behavior. For
T > 0 the solver aggregates the cavity warning configurations with the Gibbs
weight e^(-min(p,q)/T). Here min(p,q) counts the conflicting warning pairs of
one configuration. The map T = 1/y relates the deformation to the
finite-energy SP(y) equations.

```bash
./build/main --cav-temp=0.1 -w 3 4.0 50
```

The unit test `build/bsp-test` holds the numeric identities of the deformation.
The script `tools/regression_thermo.sh` rebuilds the tree and compares three
golden solver runs stored in `tests/golden`. Numbers match within 1e-12
absolute and 1e-6 relative, and all other text matches exactly. Operate the
script after each change to the solver.

```bash
./tools/regression_thermo.sh
```

## Memory checking with Valgrind

Valgrind runs on **Linux** only (not available as a native macOS/Homebrew bottle).
Prefer a Debug build without native-arch tuning:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBSP_NATIVE_ARCH=OFF
cmake --build build -j

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  ./build/main -w 3 3.0 30
```

On macOS, the same check can be run in a Linux container, for example:

```bash
docker run --rm -v "$PWD":/src -w /src ubuntu:24.04 bash -lc '
  apt-get update -qq && apt-get install -y -qq g++ cmake make valgrind
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBSP_NATIVE_ARCH=OFF
  cmake --build build -j
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    ./build/main -w 3 3.0 30
'
```
