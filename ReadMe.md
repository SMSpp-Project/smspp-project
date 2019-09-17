The SMS++ Project {#mainpage}
=================

"Umbrella" project that just unifies many different projects related to the
SMS++ structured modeling system as sub-projects.

Current sub-projects:

- [the "core" SMS++](https://gitlab.com/frangio68/sms_plus_plus)

- [S[imple/tructured]MILPBlock](https://gitlab.com/frangio68/sms_plus_plus_smilpblock)

- [MCFBlock / MCFSolver](https://gitlab.com/frangio68/sms_plus_plus_mcfblock_mcfsolver)

- [MILPSolver](https://gitlab.com/niccolo/milpsolver)

- [UCBlock](https://gitlab.com/AliGhezel/ucblock)

- [BundleSolver](https://gitlab.com/egorgone/bundlesolver)

- [LukFiBlock](https://gitlab.com/egorgone/lukfiblock)


Also, some sub-projects require further projects

- [the MCFClass project](https://github.com/frangio68/Min-Cost-Flow-Class)
  for MCFSolver

- [the NDOSolver/FiOracle project](https://gitlab.com/frangio68/ndosolver_fioracle_project)

The main benefit of this umbrella project is a unified Doxygen documentation
that can be produced by running doxygen into doxygen/.

Download and build
-------------------
If you want to download the whole project (and you have access to all the repositories):

    git clone --recurse-submodules --remote-submodules https://gitlab.com/frangio68/sms_plus_plus_project.git

Otherwise, you can clone the umbrella project and then clone only the desired submodules:

    git clone https://gitlab.com/frangio68/sms_plus_plus_project.git
    git submodule init
    git submodule update module1 module2 ...

You will also need to comment out the missing submodules from `CMakeLists.txt` 

To build the project using [CMake](https://cmake.org/download/) do the following:

    cd sms_plus_plus_project
    mkdir build
    cd build
    cmake ..
    make

After that, the `build` directory will contain all the libraries and test executables.

Legal Stuff
===========

Standard Disclaimer
-------------------

The code is currently provided free of charge for academic purposes only.
As such, it is provided "as is", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.

License
-------

See each sub-project for details.


Software Dependencies
=====================

See each sub-project for details.


Authors
=======

Lead Authors (umbrella alone)
-----------------------------

	Antonio Frangioni
	Operations Research Group
	Dipartimento di Informatica
	Universita' di Pisa
 
Contributors
------------
