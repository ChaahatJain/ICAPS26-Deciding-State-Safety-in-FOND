# PlaJA

Repository of the ***PlaJA*** (*Planning in JANI*) code base.

## Input 

Models are to be provided in the automata-based input langauge [JANI](https://jani-spec.org).\
Neural networks (where applicable) are to be provided in [NNet](https://github.com/sisl/nnet) format.\
Additionally, the [policy learning module](policy_learning) uses the [PyTorch](https://pytorch.org) format (*.pt*).\
The interface between a JANI model and a neural network is to be documented in [Jani2NNet](search/information/jani2nnet/jani2nnet_interface.jani2nnet) format.
Tree ensemble policies are to be provided in JSON format.

## Dependencies

PlaJA uses the [nlohmann JSON library](https://github.com/nlohmann/json) for parsing JANI inputs.\
The [predicate abstraction module](search/predicate_abstraction) (and other SMT modules) depend on
[Z3](https://github.com/Z3Prover/z3) and 
a [modified version](https://gitlab.cs.uni-saarland.de/vinzent/MarabouPublic.git) of [Marabou](https://github.com/NeuralNetworkVerification/Marabou).\
Marabou depends on [Boost](https://www.boost.org) & [CxxTest](https://github.com/CxxTest/cxxtest)
and (optionally) [OpenBLAS](https://www.openblas.net) & [Gurobi](https://www.gurobi.com).
(Using both is the only configuration actively maintained though.)\
The [policy learning module](policy_learning) depends on [PyTorch's](https://pytorch.org) C++ distribution [LibTorch](https://pytorch.org/cppdocs/installing).
To support tree ensemble policies, [Veritas](https://github.com/laudv/veritas.git) is required. For BDD based computations, [CUDD](https://github.com/ivmai/cudd.git) is required.
<!--- The testing infrastructure depends on [CxxTest](https://github.com/CxxTest/cxxtest). --->
<!--- Some modules under construction depend on [pybind11](https://pybind11.readthedocs.io/en/stable/index.html). --->

PlaJA has been successfully build & run with the following versions of the dependencies listed above (on Linux and MacOS):
- nlohmann JSON library version [3.11.2](https://github.com/nlohmann/json/tree/v3.11.2)
- Z3 version [4.8.12](https://github.com/Z3Prover/z3/tree/z3-4.8.12) 
commit [3a402ca2c14c3891d24658318406f80ce59b719f](https://github.com/Z3Prover/z3/tree/3a402ca2c14c3891d24658318406f80ce59b719f)
(July 13, 2021)
- Boost version [1.81.0](https://sourceforge.net/projects/boost/files/boost/1.81.0/boost_1_81_0.tar.gz) on Linux and on MacOS.
- OpenBlas version [0.3.19](https://github.com/xianyi/OpenBLAS/releases/download/v0.3.19/OpenBLAS-0.3.19.tar.gz).
- Gurobi version [12.0.0](https://www.gurobi.com/downloads/gurobi-optimizer-eula/).
- LibTorch version [1.8.1](https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.8.1%2Bcpu.zip) on Linux and [1.12.1](https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.12.1.zip) on MacOS.
- CxxTest version [4.4](https://github.com/CxxTest/cxxtest/tree/4.4) commit [191adddb3876ab389c0c856e1c03874bf70f8ee4](https://github.com/CxxTest/cxxtest/tree/191adddb3876ab389c0c856e1c03874bf70f8ee4) (June 4, 2014).
- Veritas commit (commit 1df3fc14b57ca58a62a5a77bfdf7e69beb941f5a)
- CUDD version[3.0.0](https://github.com/ivmai/cudd.git) 
<!--- - pybind11 version [2.7.1](https://github.com/pybind/pybind11/tree/v2.7.1) commit [787d2c88cafa4d07fb38c9519c485a86323cfcf4](https://github.com/pybind/pybind11/tree/787d2c88cafa4d07fb38c9519c485a86323cfcf4) (August 3, 2021) on Linux and
[2.8.0](https://github.com/pybind/pybind11/tree/v2.8.0) commit [97976c16fb7652f7faf02d76756666ef87adbe7d](https://github.com/pybind/pybind11/tree/97976c16fb7652f7faf02d76756666ef87adbe7d) (October 4, 2021) on MacOS. --->

## Building

The code base is built using [CMake](https://cmake.org).
See [CMakeLists.txt](CMakeLists.txt) and [DependenciesConfig.cmake](DependenciesConfig.cmake) for configuration.
Most notably, in the former you can enable/disable modules.
This makes sense, if you are not willing to install dependencies of modules not required for your purposes.
For instance, if the *policy learning* functionality is not needed, then you can disable it such that you do not have to install *PyTorch*.

Consider using [cmake.sh](./cmake.sh), which allows to specify a build type (Release, Debug, ...) and a build directory.
It will use "CodeBlocks - Unix Makefiles". 
You then have to run ```make``` in the respective build directory.
Alternatively, you might adapt the scripts to, e.g., use [Ninja](https://ninja-build.org/).
Here, you have to run ```ninja``` in the respective directory.

The cmake infrastructure has been successfully tested on Linux and MacOS.


## Running

After building the ***PlaJA*** executable can be found in the respective source directory.

To see basic commandline options you can run
```
./PlaJA --help
```
If you **additionally** set the engine option then commandline options specific to that engine will be provided.
For instance,
```
./PlaJA --help --engine FAULT_ANALYSIS
```
will additionally output the commandline options specific to the Fault Analysis engine.

PlaJA supports *additional properties* provided in an *additional-properties-file* which is used for fault analysis
```
./PlaJA --model-file model.jani --additional-properties additional_properties.jani --prop 1 --engine FAULT_ANALYSIS
```
The additional properties file must contain a single JSON object composed of a JANI properties array.
PlaJA will treat this array as an extension of the properties array in the model file (if present).

# Please have a look at the example_experiment scripts to understand the complete call for fault analysis.

## References & Credits

PlaJA implements a basic search engine infrastructure adapting an [extract](search/fd_adaptions) of a [stripped Fast Downward code base](https://gitlab.cs.uni-saarland.de/fai/teaching/FD-Stripped.git) (non-public).

The [probabilistic search module](search/prob_search) is an adaption of the [Probabilistic Fast Downward implementation](http://fai.cs.uni-saarland.de/downloads/fd-prob.zip).

The [successor generation](search/successor_generation) has been designed under the impression of [PRISM](https://github.com/prismmodelchecker/prism).

To allow disabling the Marabou dependency when SMT support is not required,
there is a reimplementation of Marabou's [neural network parsing functionality](parser/nn_parser).\
Moreover, for customization purposes, PlaJA also adapts Marabou's [input query generation](search/smt/nn/nn_in_marabou.cpp).

Some [auxiliary classes](utils/fd_imports) are plain [Fast Downward](https://github.com/aibasel/downward) imports,
while other adapt from similar routines [Fast Downward](utils/priority_queue.h) and [Marabou](utils/floating_utils.h).

For further notes see the reference files in the respective directories.


## LICENSE

PlaJA is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

PlaJA is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.
