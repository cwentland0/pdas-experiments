# Overview

This repository supplies experiment automation functionality for monolithic and domain-decomposed fluid flow ODEs as constructed by the [pressio-demoapps](https://github.com/Pressio/pressio-demoapps) solver, via the Schwarz decomposition framework [pressio-demoapps-schwarz](https://github.com/cwentland0/pressio-demoapps-schwarz) and the [pressio](https://github.com/Pressio/pressio) projection-based ROM utilities. This is largely a modification (with heavy copying) of the [pressio-tutorials](https://github.com/Pressio/pressio-tutorials) methodology, with extensions for the domain decomposition utilities and simplifications where possible.

# Building

Building the runner executable requires (at minimum) a C++17-compatible compiler, and a copy of both the **pressio-demoapps** (which includes the **pressio** and **Eigen** header files) and **pressio-demoapps-schwarz** source. This repository includes the C++ YAML library, which is built along with the runner executable.

```
export CXX=<path-to-your-CXX-compiler>
export PDA=<path-to-pressio-demoapps-root>
export PDAS=<path-to-pressio-demoapps-schwarz-root>

git clone git@github.com:cwentland0/pdas-experiments.git
cd pdas-experiments && mkdir build && cd build
cmake -DPDA_SOURCE=${PDA} -DPDAS_SOURCE=${PDAS} ..
make
```

# Running examples

TODO

# Reproducibility

TODO