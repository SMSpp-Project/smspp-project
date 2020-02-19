The SMS++ Project
=================

Splash page of the SMS++ Project, an "umbrella project" meant to provide a
quick way to download and install all the projects related to the SMS++
framework. It also allows to produce an unified documentation and to track
issues that involve all the modules or the project in general.

Documentation
-------------

The documentation of all modules of the SMS++ Project, automatically kept
up-to-date thanks to the CD/CI features of GitLab, will always be available
on [GitLab.io](https://smspp.gitlab.io).

Current projects
----------------

- [SMS++ core library](https://gitlab.com/smspp/smspp), the repository
  defining the general SMS++ framework features

- [BundleSolver](https://gitlab.com/smspp/bundlesolver), a Solver for
  optimization problems involving (several) nondifferentiable objective
  function(s) based on the (generalized) "bundle method". It currently
  uses some modules from the [NDOSolver / FiOracle
  project](https://gitlab.com/frangio68/ndosolver_fioracle_project)

- [LukFiBlock](https://gitlab.com/smspp/lukfiblock), a simple Block defining
  several test functions from the literature for NonDifferentiable
  Optimization solvers (such as BundleSolver)

- [MCFBlock / MCFSolver](https://gitlab.com/smspp/mcfblock), defining the
  MCFBlock class for the (continuous, linear) Min-Cost Flow problem and
  its associated MCFSolver, basically a wrapper for solvers from the
  [MCFClass project](https://github.com/frangio68/Min-Cost-Flow-Class)

- [MILPSolver](https://gitlab.com/smspp/milpsolver), defining the
  general MILPSolver Solver that aims at being able to solve any Block whose
  abstract representation encodes for a Mixed-Integer Linear Program
  (ColVariable, FRowConstraint and FrealObjective with LinearFunction inside,
  OneVarConstraint), together with derived MILPSolver classes that actually
  interface with existing MILP solvers. Currently available derived classes are

  - CPXMILPSolver, interfacing with the commercial, state-of-the-art [IBM ILOG
    Cplex](https://www.ibm.com/products/ilog-cplex-optimization-studio)

  - SCIPMILPSolver, interfacing with the open-source, state-of-the-art
    [SCIP solver](https://scip.zib.de/)

- [SDDPBlock](https://gitlab.com/smspp/sddpblock), defining the SDDPBlock for
  multi-stage stochastic optimization problems solvable by the Stochastic
  Dual Dynamic Programming approach, and the SDDPSolver that interfaces with
  the SDDP solver in the [StOpt
  project](https://gitlab.com/stochastic-control/StOpt)

- [S[imple/tructured]MILPBlock](https://gitlab.com/smspp/smilpblock), two
  very basic Block defining "a leaf MILP formulation" (without any inner
  Block) and "a structured MILP formulation with inner Block"

- [StochasticBlock](https://gitlab.com/smspp/stochasticblock), defining the
  StochasticBlock "meta-Block" that takes *any* "deterministic" Block and
  "makes it stochastic" by allowing to changing some of its data in a very
  general and abstract way (using the SMS++ "methods factory")

- [UCBlock](https://gitlab.com/smspp/ucblock), defining several Block for
  Unit Commitment problems (the general UCBlock "root" class, several Block
  for specific generating units, with some spacialized Solver, and some
  Block for specific network constraints)

- [tests](https://gitlab.com/smspp/tests), defining (complex) testers for
  several components of the project that require elements (Block and/or
  Solver) from different sub-projects and that therefore are better not
  included in any specific sub-project

- [tools](https://gitlab.com/smspp/tools), defining some tools that can be
  useful for users (such as "main files" that take instances of problems
  and solve them) and that require elements (Block and/or Solver) from
  different sub-projects so that they are better not included in any specific
  sub-project.

Getting started
---------------

These instructions will let you build the projects on your local machine.

*Requirements*

See the individual projects.

*Getting the code*

Getting the whole umbrella project and all the sub-projects can be done with
just

```sh
git clone --recursive https://gitlab.com/smspp/smspp-project
```

Otherwise, it is possible to fetch only the umbrella project with

```sh
git clone https://gitlab.com/smspp/smspp-project
```

and then use the [`get_all.sh`](get_all.sh) script to fetch all the submodules:

```sh
cd sms_plus_plus_project
./get_all.sh
```

This allows to edit the script and comment away/delete unwanted ones. This is
useful in particular if you don't have the permissions to fetch some of them.
You will be asked for your credentials multiple times, to avoid that configure
Git to store the credentials temporarily:

```sh
git config --global credential.helper cache
```

*Build and install with Cmake*

Using Cmake it is possible to configure and build all the projects at once
with:

```sh
mkdir build
cd build
cmake ..
make
```

Optionally install the libraries in the system with:

```sh
sudo make install
```

*Build and install with makefiles*

Most modules, and in particular the "core" SMS++ classes, also come with
hand-made makefiles. Using them requires to dabble with some make editing,
but is it independent on cmake.

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
LukFiBlock/test
MCFBlock/test
tests/PolyhedralFunction
tests/PolyhedralFunctionBlock
````

Contributing
------------

This section is not ready yet.

Authors
-------

These authors are for the umbrella project alone,
check the individual projects for their respective authors.

- **Antonio Frangioni**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Niccolò Iardella**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa


License
-------

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html) -
see the [LICENSE](LICENSE) file for details.
