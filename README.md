# SMS++ System Tests

A set of system tests for the SMS++ library and other modules.

Since most of the tests we devised for the SMS++ project require multiple
modules, shipping them with a single module would add unnecessary requirements
to that module. For this reason, we ship them in a separate repository.

The following tests are provided:

- [`CPXMILPSolver`](CPXMILPSolver) - solve a `SimpleMILPBlock` with a `CPXMILPSolver`
- [`MCFMILP`](MCFMILP) - solve a `MCFBlock` with both a `MILPSolver` and an `MCFSolver` and compare the results, test the Modifications.
- [`PolyhedralFunction`](PolyhedralFunction)
- [`PolyhedralFunctionBlock`](PolyhedralFunctionBlock)
- [`TUBMILP`](TUBMILP) - solve a `ThermalUnitBlock` with a `MILPSolver`

The tests run as traditional command line executables.
Some tests are provided also as [Google Test] suites.

## Getting started

These instructions will let you build and run the SMS++ System Tests
on your system.

### Requirements

- See each test for its requirements.
- Some tests require [Google Test].

[Google Test]: https://github.com/google/googletest

> If you build the tests with CMake, Google Test will be fetched and built
> automatically.

### Build

Configure and build all the tests using CMake:

```sh
mkdir build
cd build
cmake ..
make
```

Alternatively you can use the plain makefiles inside each directory.

## Usage

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

## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html) -
see the [LICENSE](LICENSE) file for details.

[Google Test]: https://github.com/google/googletest
