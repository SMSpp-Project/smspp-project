# The SMS++ Project

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

Also, some sub-projects require further projects:

- [the MCFClass project](https://github.com/frangio68/Min-Cost-Flow-Class) for MCFSolver
- [the NDOSolver/FiOracle project](https://gitlab.com/frangio68/ndosolver_fioracle_project)

The main benefit of this umbrella project is a unified Doxygen documentation
that can be produced by running doxygen into doxygen/.

## Fetch and build

- Clone the repository with:
```sh
git clone https://gitlab.com/frangio68/sms_plus_plus_project.git
```

- You can use the [`get_all.sh`](get_all.sh) script to fetch all the submodules.
```sh
cd sms_plus_plus_project
./build_all.sh
```

  If you don't have the permissions to fetch some of the submodules, comment them
  out from the script. You will be asked for your credentials multiple times, to
  avoid that configure your `git` client to store the credentials temporarily:
```sh
git config --global credential.helper cache
```

- If you have all the dependencies installed in your system, you can configure
  and build all the projects at once with:
```sh
mkdir build
cd build
cmake ..
make
```

- If the dependencies are not installed, or they are not installed in the default
  system directories, you can specify custom paths in the [`CMakeCustom.txt`](CMakeCustom.txt)
  file, e.g.:
```cmake
...
# Boost main directory
set(BOOST_ROOT  "/my/custom/path/to/boost/")
...
```

  See [`CMakeCustom.txt`](CMakeCustom.txt) for other ways to customize the configuration of SMS++ projects.

  > **Note:** Each subproject has its own [`CMakeCustom.txt`](CMakeCustom.txt) file, but if you configure and build
  > everything from this umbrella project the individual customization files will be ignored.

- Optionally, you can install the libraries in the system with:
```sh
sudo make install
```


## Legal Stuff

### Standard Disclaimer

The code is currently provided free of charge for academic purposes only.
As such, it is provided "as is", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.

### License

See each sub-project for details.


## Software Dependencies

See each sub-project for details.


## Authors

### Lead Authors (umbrella alone)

	Antonio Frangioni
	Operations Research Group
	Dipartimento di Informatica
	Universita' di Pisa

### Contributors
