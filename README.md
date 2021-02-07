# The SMS++ Project

![To boldly model (and solve) what no one has modeled (and solved) before](doxygen/SMSpp_logo_mid_noback.png)

This is the splash page of the SMS++ Project, an "umbrella project" meant
to provide a quick way to download and install all the projects related to the
SMS++ framework.
It also allows to produce an unified documentation and to track
issues that involve all the modules or the project in general.

SMS++ is a set of C++ classes intended to provide a system for modeling complex,
block-structured mathematical models (in particular, but not exclusively,
single-real-objective optimization problems), and solving them via
sophisticated, structure-exploiting algorithms (in particular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

At any given time, this project will point to the latest releases of all the
submodules.

> **Note:**
> If you are looking for the *SMS++ core library*, you will find it
> [here](https://gitlab.com/smspp/smspp).

## Documentation

- The [main documentation](https://smspp.gitlab.io/smspp-project)
  includes the API reference for all the SMS++ modules.
- The [SMS++ Project Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
  contains a more detailed description of SMS++ core concepts and features,
  as well as troubleshooting information and additional guides for
  developers and maintainers.

## Current projects

- [SMS++ core library](https://gitlab.com/smspp/smspp), the repository
  defining the general SMS++ framework features.

- [BundleSolver](https://gitlab.com/smspp/bundlesolver), a Solver for
  optimization problems involving (several) nondifferentiable objective
  function(s) based on the (generalized) "bundle method". It currently
  uses some modules from the [NDOSolver / FiOracle
  project](https://gitlab.com/frangio68/ndosolver_fioracle_project).
  
- [DPSolver](https://gitlab.com/smspp/dpsolver), a Dynamic Programming Solver
  for single Unit Commitment problems.

- [LagrangianDualSolver](https://gitlab.com/smspp/lagrangiandualsolver), a
  "generic" Lagrangian-based Solver for Block with appropriate structure.

- [LukFiBlock](https://gitlab.com/smspp/lukfiblock), a simple Block defining
  several test functions from the literature for NonDifferentiable
  Optimization solvers (such as BundleSolver).

- [MCFBlock / MCFSolver](https://gitlab.com/smspp/mcfblock), defining the
  MCFBlock class for the (continuous, linear) Min-Cost Flow problem and
  its associated MCFSolver, basically a wrapper for solvers from the
  [MCFClass project](https://github.com/frangio68/Min-Cost-Flow-Class).

- [MILPSolver](https://gitlab.com/smspp/milpsolver), defining the
  general MILPSolver Solver that aims at being able to solve any Block whose
  abstract representation encodes for a Mixed-Integer Linear Program
  (ColVariable, FRowConstraint and FrealObjective with LinearFunction inside,
  OneVarConstraint), together with derived MILPSolver classes that actually
  interface with existing MILP solvers. Currently available derived classes are:

  - CPXMILPSolver, interfacing with the commercial, state-of-the-art [IBM ILOG
    Cplex](https://www.ibm.com/products/ilog-cplex-optimization-studio);

  - SCIPMILPSolver, interfacing with the open-source, state-of-the-art
    [SCIP solver](https://scip.zib.de/).

- [MMCFBlock](https://gitlab.com/smspp/mmcfblock), defining the MMCFBlock
  Block for representing Multicommodity Min-Cost Flow problems (MMCF).

- [SDDPBlock](https://gitlab.com/smspp/sddpblock), defining the SDDPBlock for
  multi-stage stochastic optimization problems solvable by the Stochastic
  Dual Dynamic Programming approach, and the SDDPSolver that interfaces with
  the SDDP solver in the
  [StOpt project](https://gitlab.com/stochastic-control/StOpt).

- [StochasticBlock](https://gitlab.com/smspp/stochasticblock), defining the
  StochasticBlock "meta-Block" that takes *any* "deterministic" Block and
  "makes it stochastic" by allowing to changing some of its data in a very
  general and abstract way (using the SMS++ "methods factory").

- [UCBlock](https://gitlab.com/smspp/ucblock), defining several Block for
  Unit Commitment problems (the general UCBlock "root" class, several Block
  for specific generating units, with some spacialized Solver, and some
  Block for specific network constraints).

- [tests](https://gitlab.com/smspp/tests), defining (complex) testers for
  several components of the project that require elements (Block and/or
  Solver) from different sub-projects and that therefore are better not
  included in any specific sub-project.

- [tools](https://gitlab.com/smspp/tools), defining some tools that can be
  useful for users (such as "main files" that take instances of problems
  and solve them) and that require elements (Block and/or Solver) from
  different sub-projects so that they are better not included in any specific
  sub-project.


## Getting started

These instructions will let you build the projects on your local machine.


### Requirements

See the individual projects.


### Getting the code

Getting the whole umbrella project and all the sub-projects can be done with:

```sh
git clone --recurse-submodules https://gitlab.com/smspp/smspp-project
```

#### Getting the develop branches

If you feel frisky, you can try the latest, unreleased code by checking out
the develop branches:

```sh
git submodule foreach --recursive "git checkout develop || git checkout master"
```

### Build and install with CMake

Using CMake it is possible to configure and build all the projects at once
with:

```sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

Optionally install the libraries in the system with:

```sh
sudo make install
```


### Build and install with makefiles

Most modules, and in particular the "core" SMS++ classes, also come with
carefully hand-crafted makefiles. Using them requires to dabble with some
make editing, but it is independent on CMake.

The main step is to edit the makefiles into extlib/. There is one for each
of the external libraries that any module requires (starting with Boost,
Eigen and netCDF that are required by the core library and therefore by
everyone). Setting the

```make
lib*INC = -I<paths to include files directories>
lib*LIB = -L<paths to lib files directories> -l<libs>
```

in each allows one to set any non-standard path if the library is not
installed in the system (or leave them empty if they are).

The "core" SMS++ classes have a makefile for building the corresponding
library in

```sh
SMS++/lib/makefile-lib
```

The makefile allow to choose the compiler name and the optimization/debug.
This builds the lib/libSMS++.a that can be linked upon. Also, the

```sh
SMS++/lib/makefile-inc
```

file is provided for allowing external makefiles to ensure that the library
is up-to-date (useful in case one is actually developing it). The simplest
way to learn how to use it is to check the makefiles for testers, such as
those in

```sh
MCFBlock/test
tests/LagBFunction 
tests/PolyhedralFunction
tests/PolyhedralFunctionBlock
```

## Getting help

If you need support, you want to submit bugs or propose a new feature for a
individual module, see the *Getting help* section for that module.
If you need support on the project installation or want propose a new module,
you can [open a new issue](https://gitlab.com/smspp/smspp-project/-/issues/new).

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of
conduct, and the process for submitting merge requests to us.
To contribute to the individual projects, see the Contribute section for those.

## Authors

These authors are for the umbrella project alone, check the individual
projects for their respective authors.

- **Antonio Frangioni**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Niccolò Iardella**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html) -
see the [LICENSE](LICENSE) file for details.

## Disclaimer

The code is currently provided free of charge under an open-source license.
As such, it is provided "*as is*", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.
