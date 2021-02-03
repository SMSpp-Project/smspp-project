# SMS++ System Tests

A set of system tests for the the SMS++ core library and several other
modules.

Since most of the tests we devised for the SMS++ project require multiple
modules, shipping them with a single module would add unnecessary requirements
to that module. For this reason, we ship them in a separate repository.

The following tests are provided:

- [`BendersBFunction`](BendersBFunction)

- [`BoxSolver`](BoxSolver), a tester which provides very
  comprehensive tests for `BoxSolver` (a very simple `CDASolver` for
  extremely simple problems where each `ColVariable` can
  be dealt with separately subject only to bound and integrality
  constraints and a linear or quadratic `Objective`, ignoring any other
  kind of `Constraint` if they are there) as well as to any `CDASolver`
  able to handle Linear Programs (such as `MILPSolver` and its derived
  classes `CPXMILPSolver` and `SCIPMILPSolver`), and for some of the
  mechanics of the SMS++ core library.

- [`LagBFunction`](LagBFunction), a tester which provides very
  comprehensive tests for `LagBFunction`, `PolyhedralFunctionBlock`,
  `PolyhedralFunction`, any `CDASolver` able to handle `C05Function` in the
  objective (such as `BundleSolver`, for which some specific provisions are
  made), any `CDASolver` able to handle Linear Programs (such as `MILPSolver`
  and its derived classes `CPXMILPSolver` and `SCIPMILPSolver`), as well as
  for quite a lot of the mechanics of the SMS++ core library.

- [`LagrangianDualSolver_Box`](LagrangianDualSolver_Box), a tester
  which provides very comprehensive tests for `LagrangianDualSolver`,
  `LagBFunction`, `BoxSolver`, any `CDASolver` able to handle `C05Function`
  in the `Objective`, any `CDASolver` able to handle Linear Programs (such
  as `MILPSolver` and its derived classes `CPXMILPSolver` and
  `SCIPMILPSolver`), as well as for quite a lot of the mechanics of the
  SMS++ core library.

- [`LagrangianDualSolver_MMCF`](LagrangianDualSolver_MMCF),
  a tester which provides  initial tests for `LagrangianDualSolver`,
  `LagBFunction`, any `CDASolver` able to handle `C05Function` in the
  `Objective` (such as `BundleSolver`), any `CDASolver` able to handle
  Linear Programs (such as `MILPSolver` and its derived classes
  `CPXMILPSolver` and `SCIPMILPSolver`), `MMCFBlock` and `MCFBlock`,
  as well as for quite a lot of the mechanics of the SMS++ core library.

- [`LagrangianDualSolver_UC`](LagrangianDualSolver_UC), a tester
  which provides initial tests for `LagrangianDualSolver`, `LagBFunction`,
  any `CDASolver` able to handle `C05Function` in the `Objective` (such as
  `BundleSolver`), any `CDASolver` able to handle Linear Programs (such as
  `CPXMILPSolver` and `SCIPMILPSolver`), the `UCBlock` set of `Block`for
  Unit-Commitment problems, as well as for quite a lot of the mechanics
  of the SMS++ core library.

- [`MCF_MILP`](MCF_MILP) - solve a `MCFBlock` with both a
  `MILPSolver` and a `MCFSolver` and compare the results, test the `Modifications`.
  This is a test for `MCFBlock`, `MCFSolver`, `MILPSolver` and its derived classes
  (`CPXMILPSolver` and `SCIPMILPSolver`), as well as for some of the
  mechanics of the SMS++ core library.
  This test is a [Google Test](https://github.com/google/googletest) suite.

- [`MMCFBlock`](MMCFBlock), a tester which provides initial tests
  for `MMCFBlock` (in particular, a way to retrieve/generate some sets of
  Multicommodity Min-Cost Flow instances) and any `Solver` able to handle
  Linear Programs (such as `MILPSolver` and its derived classes
  `CPXMILPSolver` and `SCIPMILPSolver`), as well as for a few of the
  mechanics of the SMS++ core library.

- [`PolyhedralFunction`](PolyhedralFunction), a tester which
  provides very comprehensive tests for `PolyhedralFunction` and some tests
  for any `CDASolver` able to handle `C05Function` in the objective (such as
  `BundleSolver`) and any `CDASolver` able to handle Linear Programs (such
  as `MILPSolver` and its derived classes `CPXMILPSolver` and
  `SCIPMILPSolver`), as well as for some of the mechanics of the SMS++
  core library.

- [`PolyhedralFunctionBlock`](PolyhedralFunctionBlock), a tester
  which provides very comprehensive tests for `PolyhedralFunction` and
  especially `PolyhedralFunctionBlock`, plus quited some tests for any
  `CDASolver` able to handle multiple `C05Function` in the objective (such
  as `BundleSolver`) and any `CDASolver` able to handle Linear Programs
  (such as `MILPSolver` and its derived classes `CPXMILPSolver` and
  `SCIPMILPSolver`), as well as for some of the mechanics of the SMS++
  core library.

- [`UC_MILP`](UC_MILP) - solve several UC problems with a `MILPSolver`.
  This is a  test for `UCBlock`, `MILPSolver` and its derived classes
  (`CPXMILPSolver` and `SCIPMILPSolver`), as well as for some of the
  mechanics of the SMS++ core library.
  This test is a [Google Test](https://github.com/google/googletest) suite.


The tests run as traditional command line executables.
Most of the tests can also run as a 
[CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) suites.


## Getting started

These instructions will let you build and run the SMS++ System Tests
on your system.


### Requirements

- See each test for its requirements.

- Some tests require [Google Test](https://github.com/google/googletest).

> If you build the tests with CMake, Google Test will be fetched and built
> automatically.


### Build with CMake

Configure and build all the tests using CMake:

```sh
mkdir build
cd build
cmake ..
make
```

### Build and install with makefiles

Carefully hand-crafted makefiles have also been developed for those unwilling
to use CMake. General instructions are:

- The arrangements of folders must be that envisioned by the
  [Umbrella SMS++ Project](https://gitlab.com/smspp/smspp-project)

- The main step is to edit the makefiles into ../extlib/. There is one for
  each of the external libraries that any module requires, starting with
  Boost, Eigen and netCDF-C++. Setting the

```make
lib*INC = -I<paths to include files directories>
lib*LIB = -L<paths to lib files directories> -l<libs>
```

  in each allows one to set any non-standard path if the library is not
  installed in the system (or leave them empty if they are).

- For each test that has a makefile, you can chdir the corresponding directory
  and run make. However, note that the "basic" makefile macros

```make
CC =
SW =
```

  for setting the c++ compiler and its options are defined in the makefile and
  "automatically forwarded" to these of the other SMS++ components, so that
  (possibly at the cost of a make clean) consistency is ensured during the
  building process; thus, editing the makefile and changig these may also be
  required.

## Usage

Each tester has an executable built in the corresponding directory; run
it for instructions. In several cases a (bash) batch is available to run
a default sequence of tests (this may take a while).


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of
conduct, and the process for submitting merge requests to us.


## Authors

- **Antonio Frangioni**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Ali Ghezelsoflu**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Enrico Gorgone**  
  Dipartimento di Matematica ed Informatica  
  Università di Cagliari

- **Niccolò Iardella**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Rafael Durbano Lobato**  
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
