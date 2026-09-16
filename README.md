# QASC

## Introduction
QASC (Quantum Assembly Compiler) is a high-performance source-to-source compiler designed for hybrid quantum-classical computing. It translates OpenQASM 3.0 programs into
highly optimised C++ code, emitting classical instructions natively and routing quantum operations directly to the [QuEST](https://github.com/QuEST-Kit/QuEST) state-vector simulator.

By compiling down to a standard C++ binary, QASC allows developers to test, benchmark, and validate complex hybrid workflows portably, leveraging high-performance resources. It bridges the gap between theoretical algorithm design and physical deployment, ensuring user workflows are fully verified and ready for integration with real quantum computing hardware.

## Dependencies
- cmake
- clang++19 (or higher) / g++13 (or higher) / Intel OneAPI C++ 2026
- Antlr4 (v4.13.2)
- QuEST  (v4.2.0+)

## Building
```
CC=<c-comp> CXX=<cpp-comp> cmake -G <gen> -S . -B build (optional) -DEXTRA_RPATH_DIRS="<path1>:<path2>:..."
cmake --build build
```

The project will pull and build Antlr4 as a dependency.

By default the project will look for QuEST in the usual system/home directories.
If it fails to find a valid QuEST installation it will pull and build v4.2.0 release
from GitHub.

## Running
Users can override the QuEST library and include paths with environment variables.
In case when user does not want to `make install` QuEST (but just have a local build),
user should export environment variables.

```
export DEPS_QUEST_INCLUDE_DIR=<path-to-quest-include>
export DEPS_QUEST_LIB_DIR=<path-to-quest-lib>
```

Additionally, users can provide command-line flags to override the default libraries
(e.g. QuEST, MPI if QuEST was built in distributed mode etc.).
User specified paths and libraries take precedences over those set during installation.

```
Usage: qasc [--help] [--version] -cxx VAR --input VAR --output VAR [-L VAR]... [-l VAR]... [-I VAR]... [-F] [--verbose]

Optional arguments:
  -h, --help     shows help message and exits
  -v, --version  prints version information and exits
  -cxx           specify standard C++ compiler. [required]
  -i, --input    specify the input file. [required]
  -o, --output   specify the output file. [required]
  -L             specify library paths. [nargs=0..1] [default: {}] [may be repeated]
  -l             specify libraries. [nargs=0..1] [default: {}] [may be repeated]
  -I             specify include paths. [nargs=0..1] [default: {}] [may be repeated]
  -F             set output cpp formating (requires clang-format).
  -V, --verbose  set qasc program execution to verbose mode.
```

## Example
```
$ qasc -cxx clang++ -i examples/qft.qasm -o qft
$ ./qft

$ cat qft.result
```

## Running tests
```
cd build
make test

# or 

./build/tests/test_qasc
```

## Authors and Citation
QASC white paper is in preparation. In the meantime this work can be acknowledged by citing the companion paper which used QASC to validate hybrid quantum chemistry workflows.
```
@article{10.1021/acs.jctc.6c00542,
    author = {Thacker, Joseph C. R. and Meller, Mateusz and Popescu, Alberto and Patterson, Andrew and Izsák, Róbert and Szeremi, Vendel and Keal, Thomas W.},
    title = {Hybrid HPC and Quantum Computing Framework for Multiscale Modeling of Chemical Systems},
    journal = {Journal of Chemical Theory and Computation},
    year = {2026},
    month = {08},
    issn = {1549-9618},
    doi = {10.1021/acs.jctc.6c00542},
    url = {https://doi.org/10.1021/acs.jctc.6c00542},
    eprint = {https://pubs.acs.org/jctcce/article-pdf/doi/10.1021/acs.jctc.6c00542/66406147/acs.jctc.6c00542.pdf},
}
```
