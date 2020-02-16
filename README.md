# The SMS++ Project

Splash page of the SMS++ Project, an "umbrella project" meant to provide a
quick way to download and install all the projects related to the SMS++
framework. It also allows to produce an unified documentation and to track
issues that involve all the modules or the project in general.

## Documentation

The documentation of all modules of the SMS++ Project, automatically kept
up-to-date thanks to the CD/CI features of GitLab, will always be available
on [GitLab.io](https://smspp.gitlab.io).

## Current projects

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
  OneVarConstraint), together with derived *MILPSolver classes that actually
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
  sub-project

## Getting started

These instructions will let you build the projects on your local machine.

### Requirements

See the individual projects.

### Build and install

You can use the [`get_all.sh`](get_all.sh) script to fetch all the submodules:
```sh
cd sms_plus_plus_project
./get_all.sh
```

If you don't have the permissions to fetch some of the submodules, comment them
out from the script. You will be asked for your credentials multiple times, to
avoid that configure Git to store the credentials temporarily:
```sh
git config --global credential.helper cache
```

Configure and build all the projects at once with:
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

## Contributing

This section is not ready yet.

## Authors

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

## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html).
