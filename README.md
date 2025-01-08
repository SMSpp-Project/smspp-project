# The SMS++ Project

![To boldly model (and solve) what no one has modeled (and solved)
before](doxygen/SMSpp_logo_mid_noback.png)

This is the splash page of the `SMS++ Project`, an "umbrella project" meant to
provide a quick way to download and install all the projects related to the
`SMS++` framework. It also allows to produce a unified documentation and to
track issues that involve all the modules or the project in general.

`SMS++` is a set of `C++` classes intended to provide a system for modeling
complex, block-structured mathematical models (in particular, but not
exclusively, single-real-objective optimization problems), and solving them
via sophisticated, structure-exploiting algorithms (in particular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

At any given time, this project will point to the latest releases of all the
submodules.

> **Note:**
> If you are looking for the *SMS++ core library*, you will find it
> [here](https://gitlab.com/smspp/smspp).


## Success stories

The `SMS++` framework is written in advanced `C++`, and its use entails climbing
a significant learning curve. Yet, mastering it may be worth especially for
the solution of very-large-scale, demanding optimization problems arising
in applications. Indeed, the `SMS++ Project` has already been instrumental in
several complex projects, such as:

- The [plan4res H2020 project](https://www.plan4res.eu) that funded the first
  major expansion of `SMS++` capabilities.

- The [openENTRANCE H2020 project](https://openentrance.eu), where `SMS++`
  contributed to [the open modelling platform developed by the
  project](https://openenergymodels.net/models/plan4res-modelling-suite).

- The [OpenMod4Africa project](https://openmod4africa.eu), in which `SMS++` has
  been one of the main modelling tools for solving complex electricity system
  optimization and simulation problems in Africa, training many researchers,
  modellers and stakeholders from both western and eastern Africa on how to use
  the tools to develop case studies at the level of the two regions.

- The [CETPartnership "RESILIENT - Resilient Energy System Infrastructure
  Layouts for Industry, E-fuels and Network
  Transition"](https://resilient-project.github.io), where `SMS++` is being
  interfaced with the open-source, widely-used, multi-vector energy planning
  tool [PyPSA-Eur](github.com/pypsa/pypsa-eur) to provide solution methods for
  the more demanding energy models

Furthermore, the use of `SMS++` is planned for several other projects; two
recently accepted ones are

- The CETPartnership "Manoeuvre", where a methodology will be developed for
  linking `SMS++` with the global energy model
  [GENeSYS-Mod](http://www.osemosys.org/genesys-mod.html) to conduct case studies,
  where `SMS++` will be run on a technologies mix calculated by GENeSYS-Mod and
  will send back some indicators that will allow to refine the previous solution.

Watch this space for more details and news as they become available.


## Documentation

- The [SMS++ API reference](https://smspp.gitlab.io/smspp-project)
  contains the documentation for the classes and methods of the SMS++ core
  library and all its modules.

- The [SMS++ Project Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)
  contains detailed installation instructions, troubleshooting information
  and additional guides for developers and maintainers.


## Current projects

- [SMS++ core library](https://gitlab.com/smspp/smspp), the repository
  defining the general `SMS++` framework features.

- [BinaryKnapsackBlock](https://gitlab.com/smspp/binaryknapsackblock),
  a `Block` implementing the Binary Knapsack Problem and the corresponding
  Solver based on a straightforward implementation of the Dynamic Programming
  approach (for integer weights).

- [CapacitatedFacilityLocationBlock](https://gitlab.com/smspp/capacitatedfacilitylocationblock),
  an implementation of the `Block` concept for a "pretty basic version" the
  Capacitated Facility Location (CFL) problem, a.k.a. the Capacitated
  Warehouse Location (CWL) problem, primarily intended as a "didactic"
  implementation for showing some of the features of `SMS++`.

- [BundleSolver](https://gitlab.com/smspp/bundlesolver), a `Solver` for
  optimization problems involving (several) nondifferentiable objective
  function(s) based on the (generalized) "bundle method". It currently
  uses some modules from the [NDOSolver / FiOracle
  project](https://gitlab.com/frangio68/ndosolver_fioracle_project),
  although the dependency will be hopefully removed in time.

- [InvestmentBlock](https://gitlab.com/smspp/investmentblock), a `Block`
  designed to model the investment in different assets defined in
  [UCBlock](https://gitlab.com/smspp/ucblock). The corresponding problem
  can be either deterministic or stochastic, in which case the module also
  uses [SDDPBlock](https://gitlab.com/smspp/sddpblock) and therefore
  [StochasticBlock](https://gitlab.com/smspp/stochasticblock).
  
- [LagrangianDualSolver](https://gitlab.com/smspp/lagrangiandualsolver), a
  "generic" Lagrangian-based Solver for `Block` with appropriate structure.

- [LukFiBlock](https://gitlab.com/smspp/lukfiblock), a simple `Block` defining
  several test functions from the literature for NonDifferentiable
  Optimization solvers (such as `BundleSolver`).

- [MCFBlock / MCFSolver](https://gitlab.com/smspp/mcfblock), defining the
  `MCFBlock` class for the (continuous, linear) Min-Cost Flow problem and
  its associated `MCFSolver`, basically a wrapper for solvers from the
  [MCFClass project](https://github.com/frangio68/Min-Cost-Flow-Class).

- [MILPSolver](https://gitlab.com/smspp/milpsolver), defining the
  general `MILPSolver` `Solver` that aims at being able to solve any `Block`
  whose abstract representation encodes for a Mixed-Integer Linear Program
  (`ColVariable`, `FRowConstraint` and `FRealObjective` with `LinearFunction`
  inside, `OneVarConstraint`), together with derived classes that actually
  interface with existing MILP solvers. Currently, available derived classes
  are:

  - `CPXMILPSolver`, providing the interface with the commercial
    [IBM ILOG CPLEX Studio](https://www.ibm.com/products/ilog-cplex-optimization-studio)

  - `SCIPMILPSolver`, providing the interface with the open-source
    [SCIP](https://www.scipopt.org) (note that since version 8.0.3 SCIP is
    "truly" FOSS by dint of being distributed under the Apache 2.0 License as
    opposed to the previous academic license preventing royalty-free commercial
    use)

  - `GRBMILPSolver`, providing the interface with the commercial
    [GUROBI Optimizer](https://www.gurobi.com/solutions/gurobi-optimizer)

  - `HiGHSMILPSolver`, providing the interface with the open-source
    [HiGHS](https://highs.dev)

- [MMCFBlock](https://gitlab.com/smspp/mmcfblock), defining the `MMCFBlock`
  for representing Multicommodity Min-Cost Flow problems (MMCF). The
  current version is rather crude and in desperate need of some love.

- [SDDPBlock](https://gitlab.com/smspp/sddpblock), defining the `SDDPBlock` for
  multi-stage stochastic optimization problems solvable by the Stochastic
  Dual Dynamic Programming approach, and the `SDDPSolver` that interfaces with
  the SDDP solver in the
  [StOpt project](https://gitlab.com/stochastic-control/StOpt).

- [StochasticBlock](https://gitlab.com/smspp/stochasticblock), defining the
  `StochasticBlock` "meta-Block" that takes *any* "deterministic" `Block` and
  "makes it stochastic" by allowing to change some of its data in a very
  general and abstract way (using the `SMS++` "methods factory").

- [UCBlock](https://gitlab.com/smspp/ucblock), defining several `Block` for
  Unit Commitment problems: the general `UCBlock` "root" class, several
  `Block` for specific generating units (`UnitBlock`) and interconnect
  networks (`NetworkBlock`), with some specialized `Solver`.

- [tests](https://gitlab.com/smspp/tests), defining (complex) testers for
  several components of the project that require elements (`Block` and/or
  `Solver`) from different subprojects and that therefore are better not
  included in any specific subproject.

- [tools](https://gitlab.com/smspp/tools), defining some tools that can be
  useful for users (such as "main files" that take instances of problems
  and solve them) and that require elements (`Block` and/or `Solver`) from
  different subprojects so that they are better not included in any specific
  subproject.


## Getting started

These instructions will let you build the projects on your local machine.

### Requirements

If you are new to SMS++ and you are looking for a zero-waste way to install all
dependencies in the default locations, we suggest using the ad-hoc INSTALL
files.

If you have not yet cloned the SMS++ repository, you can launch the script directly:

```powershell
# Windows (from a PowerShell as administrator)
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
& ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1')))
```

or:

```sh
# Unix
# using curl
curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path>
# using  wget
wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path>
```

if not specified, the default installation root path is `/opt` for Linux (or
`$HOME` if the current user is not sudoer), and `/Library` for macOS.

If you do not have a CPLEX and/or Gurobi license, or if you just want to install
SMS++ without them, you can run:

```powershell
# Windows (from a PowerShell as administrator)
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
& ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1'))) -withoutCplex -withoutGurobi
```

or:

```sh
# Unix
# using curl
curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path> --without-cplex --without-gurobi
# using  wget
wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path> --without-cplex --without-gurobi
```

Otherwise, if you are already inside the cloned repository, you can run the
script via:

```powershell
# Windows (from a PowerShell as administrator)
.\INSTALL.ps1
```

or:

```sh
# Unix
sudo ./INSTALL.sh
```

using the same flag options according to the OS, if needed.

If you need more detailed instructions on how to install the project's
requirements, please refer to the [SMS++ requirements installation
guide](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++#requirements).

### Centralised path repository

You can either use [CMake](https://cmake.org) or plain makefiles to build the
library, your choice. CMake compiles off-source, and it is therefore perhaps
better suited to one-off, compile-and-forget installations, whereby the
provided makefiles compile on-source, and we find that they are better suited
while developing and testing new code (please do, this is a community project).

In both cases, all external dependencies should be automatically dealt with if
they are installed in their default paths. These are specified in the `*_ROOT`
values that are defined in the four files
[`extlib/makefile-default-paths-linux`](extlib/makefile-default-paths-linux),
[`extlib/makefile-default-paths-macos-x86_64`](extlib/makefile-default-paths-macos-x86_64),
[`extlib/makefile-default-paths-macos-arm64`](extlib/makefile-default-paths-macos-arm64),
and [`extlib/makefile-default-paths-win`](extlib/makefile-default-paths-win)
for the three major OS families; the one that gets used is automatically
selected by the build process (both with CMake and the makefiles). If not, the
suggested way to change them is to copy the (right one of these) file(s) into
`extlib/makefile-paths` and edit it. This file, if present, is automatically
read and the values found there replace the corresponding non-default definitions.
The rationale for not directly changing the makefile-default-paths-* is that
`extlib/makefile-paths` file is .gitignore-d. Hence, it should not be necessary to
re-change the makefiles (or stash/restore the changes) each time the project is
pulled, or manually ignore the changes when it is pushed, which is very convenient
for anyone who actually develops new `SMS++` components (which you know you
should, so please do).

### Build and install with CMake

If you need more detailed instructions on how to install the project with
CMake, please refer to the [SMS++ installation
guide](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++#sms).

The [`CMakeLists.txt`](CMakeLists.txt) file also provides a quick reference
on requirements and dependencies between modules.

Several CMake configuration options are available, see
[here](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration).

### Build and install with makefiles

All modules, starting with the "core" `SMS++ classes`, also come with
carefully hand-crafted makefiles. Using them may require dabbling with some
makefile editing, but it is independent on CMake and may be more suited for
anyone who is developing `SMS++` components.

In principle makefiles may work right out of the bat if all dependencies are
installed at their OS-specific default locations; see the
`extlib/makefile-default-paths-*` discussion above. If any of the libraries 
is in a nonstandard location, it should be possible to still have the make 
process to run easily by copying `extlib/makefile-default-paths-*` into 
`extlib/makefile-paths` (the file is not there because it is .gitignore-d,
precisely so that local settings are never accidentally made public and are 
not overwritten when pulling the repository again), and properly setting the 
corresponding *-ROOT values.

See the [SMS++ installation wiki](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration#location-of-required-libraries)
for further details.

The "core" `SMS++` classes have a makefile for building the corresponding
library in

```sh
SMS++/lib/makefile-lib
```

The makefile allow to choose the compiler name and the optimization/debug
settings. This builds the `lib/libSMS++.a` that can be linked upon. Also, the

```sh
SMS++/lib/makefile-inc
```

file is provided for allowing external makefiles to ensure that the library
is up-to-date (useful in case one is actually developing it). The simplest
way to learn how to use it is to check the makefiles for testers, such as
those in

```sh
MCFBlock/test
MILPSolver/test_*
tests/CapacitatedFacilityLocationBlock 
tests/LagBFunction 
tests/LagrangianDualSolver_MMCF
tests/LagrangianDualSolver_UC
tests/PolyhedralFunction
tests/PolyhedralFunctionBlock
```

and many others. Note that the [makefile-lib](SMS++/lib/makefile-lib) and
[makefile-inc](SMS++/lib/makefile-inc) "listen" to the general macros `CC`
and `SW` controlling the `C++` compiler and its main options; these can
therefore be set in the "main" makefile and will be used throughout the
whole compilation. This may be useful to set system-specific values.

An example of this is the macro

```sh
CLANG_1200_0_32_27_PATCH
```

which activates a patch for a weird glitch of `clang++` (from 1200.0.32.27
to at least 1200.0.32.29) that cause some `boost::any magic` to stop working.
Other settings may be needed (see, for instance, the comments about
`--force_link` in the [makefile of tests/BoxSolver](tests/BoxSolver/makefile)).


## First steps

Although sadly a proper User Manual is still missing, the
[tests](https://gitlab.com/smspp/tests) repository can be useful to get a
first look at possible ways of using `SMS++`. In particular the three
tests `LagrangianDualSolver_Box`, `LagrangianDualSolver_MMCF` and
`LagrangianDualSolver_UC` all build, or load from file, a `:Block` amenable
to Lagrangian relaxation, register two `Solver` (a `:MILPSolver` and a
`LagrangianDualSolver`) to it, properly configure them if needed (mostly,
but not exclusively, using `Configuration` files) and run the two `Solver`
to verify that they give the same answer. The `LagrangianDualSolver_Box`
test also shows how one can use SMS++ to build "normal" models a-la
algebraic modelling language using `AbstractBlock`, as opposed to
programming a whole `Block` from scratch, more of which can be found, e.g.,
in `tests/LagBFunction` and `tests/PolyhedralFunction`. Furthermore,
provides an example on how to (randomly) modify the `:Block` and re-solve
it many times (still checking that the results agree), showcasing the
crucial `Modification` `SMS++` concept whereby changes in the `:Block` are
automatically forwarded to all concerned `Solver`. Other similar examples
can be found in basically all the other tests.

The `LagrangianDualSolver_MMCF` and `LagrangianDualSolver_UC` versions rather
provides examples about using full-featured "pre-built" `:Block`, each of
which is "structured" and therefore has sub-`Block` inside of possibly
different types that can possibly be solved by specialised `:Solver`; c.f.,
e.g., the [MMCFBlock](https://gitlab.com/smspp/mmcfblock),
[MCFBlock](https://gitlab.com/smspp/mcfblock) and the
[UCBlock](https://gitlab.com/smspp/ucblock) repo, respectively.


## Getting help

If you need support, you want to submit bugs or propose a new feature for an
individual module, see the *Getting help* section for that module.

If you need support on the project installation, you can check out the
[installation
guide](https://gitlab.com/smspp/smspp-project/-/wikis/Installing-SMS++)
or the [troubleshooting
page](https://gitlab.com/smspp/smspp-project/-/wikis/Troubleshooting)
in our Wiki.

If your issue is not covered by our guides, or you want to propose a new
module, you can [open a new
issue](https://gitlab.com/smspp/smspp-project/-/issues/new).


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of
conduct, and the process for submitting merge requests to us. To contribute
to the individual projects, see the Contribute section for those.

Contributing entirely new subprojects, i.e., either new `:Block` for your
favourite class of optimization models and/or new `:Solver` implementing your
favourite solution method (very general or specialised to a very narrow class,
everything is useful) is extremely welcome. The `SMS++ Project` maintainers
will bend backwards to help you develop them and will be happy to host them in
the umbrella repository and integrate them with the rest of the
`SMS++ Project`. This being open source, of course it is your choice whether
you do it, if you will release your code and which license will it be
released under. However, `SMS++` is a community project, and we humbly suggest
you to consider participating in it with the rules we have been setting.
Constructive criticisms and proposals about changing these rules (the umbrella
repository organisation and whatnot) are very welcome. In fact, we believe that
the `SMS++ Project` underlines the need for a software distribution mechanism
for projects that are at the same time tightly-knit together and composed of
largely independent units that we don't seem to see around, but if we have
missed it we'd be happy for a tip.


## Authors

These authors are for the umbrella project alone. Check the individual
projects for their respective authors.

### Current lead authors

- **Antonio Frangioni**  
  Dipartimento di Informatica  
  Università di Pisa

- **Rafael Durbano Lobato**  
  Dipartimento di Informatica  
  Università di Pisa

- **Donato Meoli**  
  Dipartimento di Informatica  
  Università di Pisa

### Main past contributors

- **Niccolò Iardella**  
  Dipartimento di Informatica  
  Università di Pisa


## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html) -
see the [LICENSE](LICENSE) file for details.


## Disclaimer

The code is currently provided free of charge under an open-source license.
As such, it is provided "*as is*", without any explicit or implicit warranty
that it will properly behave, or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.


## Acknowledgements

The development of `SMS++` has greatly benefited from the contributions
of the following projects:

- "Consistent Dual Signals and Optimal Primal Solutions", funded by the
  [Gaspard Monge program for Optimization and Operations Research](http://www.fondation-hadamard.fr/fr/PGMO).

- [plan4res](https://www.plan4res.eu), grant agreement No 773897 within
  European Union's Horizon 2020 research and innovation programme.

- "Multilevel Heterogeneous Distributed Decomposition for Energy Planning
  with SMS++", funded by the
  [Gaspard Monge program for Optimization and Operations Research](http://www.fondation-hadamard.fr/fr/PGMO).

- "Optimization under Uncertainty with SMS++", funded by the
  [Gaspard Monge program for Optimization and Operations Research](http://www.fondation-hadamard.fr/fr/PGMO).
