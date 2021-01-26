# SMS++ System Tests

A set of system tests for the the SMS++ "core" library and several other
modules.

Since most of the tests we devised for the SMS++ project require multiple
modules, shipping them with a single module would add unnecessary requirements
to that module. For this reason, we ship them in a separate repository.

The following tests are provided:

- [`BendersBFunction`](BendersBFunction)

- [`LagBFunction`](LagBFunction/README.md) - constructs one `AbstractBlock` with two
  different kinds of sub-Block: some `PolyhedralFunctionBlock` (configured in
  the "natural" way where the `FRealObjective` has a `PolyhedralFunction`),
  and some with `FRealObjective` with a `LagBFunction` having as inner `Block`
  a simple transportation problem. Then, another `AbstractBlock` is
  constructed with the same inner `PolyhedralFunctionBlock` (but configured
  in "linearized" way where the `FRealObjective` has a `LinearFunction` and
  there are "linear" constraints) and some `AbstractBlock` sub-Block that
  basically represent, in a dual way, the same problem that the `LagBFunction`
  sub-Block do. The two `Block` are randomly changed in many different ways
  (in exactly the same way for the `PolyhedralFunctionBlock`, in different but
  mathematically equivalent ways for the others), then solved (the first
  typically via a `BundleSolver` and the second via a `MILPSolver`) and the
  results are checked for consistency. This is a very comprehensive test for
  `LagBFunction`, `PolyhedralFunctionBlock`, `PolyhedralFunction`,
  `BundleSolver`, `MILPSolver` and its derived classes (`CPXMILPSolver` and
  `SCIPMILPSolver`), as well as for quite a lot of the mechanics of the "core"
  SMS++ library.

- [`MCFMILP`](MCFMILP) - solve a `MCFBlock` with both a `MILPSolver` and an
  `MCFSolver` and compare the results, test the Modifications. This is a
  test for `MCFBlock`, `MCFSolver`,  `MILPSolver` and its derived classes
  (`CPXMILPSolver` and `SCIPMILPSolver`), as well as for some of the
  mechanics of the "core" SMS++ library.

- [`PolyhedralFunction`](PolyhedralFunction) - constructs an `AbstractBlock`
  with a single `PolyhedralFunction` as objective (inside a `FRealObjective`)
  and another `AbstractBlock` with a representation of the same function via
  "linear constraints". The function is randomly changed in many different
  ways, the two `Block` are solved (the first typically via a `BundleSolver`
  and the second via a `MILPSolver`) and the results are checked for
  consistency. This is a very comprehensive test for `PolyhedralFunction`
  and tests some features of `BundleSolver` and `MILPSolver` and its derived
  classes (`CPXMILPSolver` and `SCIPMILPSolver`), as well as for quite some
  mechanics of the "core" SMS++ library.

- [`PolyhedralFunctionBlock`](PolyhedralFunctionBlock) - constructs one
  `AbstractBlock` with one or more `PolyhedralFunctionBlock` and (possibly a
  `LinearFunction` as objective) and copies it; one copy is configured in
  the "natural" way (the `FRealObjective` has a `PolyhedralFunction`) and the
  other copy in the "linearized" way (the `FRealObjective` has a
  `LinearFunction` and there are "linear" constraints). The `AbstractBlock` is
  randomly changed in many different ways (and the changes are moved to the
  copy by means of `UpdateSolver` and `map\_forward\_Modification()`), the
  two `Block` are solved (the first typically via a `BundleSolver` and the
  second via a `MILPSolver`) and the results are checked for consistency.
  This is a very comprehensive test for `PolyhedralFunctionBlock` and
  `PolyhedralFunction` and tests several features of `BundleSolver`,
  `MILPSolver` and its derived classes (`CPXMILPSolver` and `SCIPMILPSolver`),
  as well as for quite a lot of the mechanics of the "core" SMS++ library.


The tests run as traditional command line executables.
Some tests are provided also as [Google Test] suites.


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

This section is not ready yet.


## Authors

- **Antonio Frangioni**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Ali Ghezelsoflu**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

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
