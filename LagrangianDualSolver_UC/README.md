# test/LagrangianDualSolver_UC

A tester which provides initial tests for `LagrangianDualSolver`,
`LagBFunction`, any `CDASolver` able to handle `C05Function` in the
`Objective` (such as `BundleSolver`), any `CDASolver` able to handle
Linear Programs (such as `MILPSolver` and its derived classes
`CPXMILPSolver` and `SCIPMILPSolver`), the `UCBlock` set of `Block`
for Unit-Commitment problems, as well as for quite a lot of the
mechanics of the "core" SMS++ library.

This executable, given the filename of a netCDF file containing the
description of a `UCBlock` (instance of the Unit-Commitment problem),
solves its Lagrangian Dual (with the continuous relaxation of the
subproblems solved, i.e., basically the Linear Dual) with a
`LagrangianDualSolver` and its continuous relaxation with a
`:MILPSolver`, comparing the results (and printing the running time).

The usage of the executable is the following:

       ./LDS_UC_test UC-file [BSC-file]
       BSC-file: BlockSolverConfig description [BSPar.txt]

A batch file is provided that runs the test on all the pure-thermal
"academic" UC instances available at

http://groups.di.unipi.it/optimize/Data/UC.html

(translated in netCDF with the translator available in the `UCBlock`
repo). The batch assumes the instances are in the sub-folder "data",
that can be symlinked from the `UCBlock` repo such as in

    ln -s ../../UCBlock/netCDF_files/UC_Data/T-Ramp data

A makefile is also provided that builds the executable including the
`LagrangianDualSolver` module, the `BundleSolver` module and all its
dependencies, in particular `MILPSolver` together of course with the
core SMS++ library, and the `UCBlock` module.

## Authors

- **Antonio Frangioni**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html) -
see the [LICENSE](LICENSE) file for details.
