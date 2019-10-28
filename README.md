# The SMS++ Project

This umbrella project provides a quick reference for all the projects related
to SMS++, and offers a quick way to retrieve and build them at once.
It also allows to produce an unified documentation and to track issues that
involve all the modules or the project in general.

## Current projects

- [SMS++ core library](https://gitlab.com/frangio68/sms_plus_plus)
- [S[imple/tructured]MILPBlock](https://gitlab.com/frangio68/sms_plus_plus_smilpblock)
- [MCFBlock / MCFSolver](https://gitlab.com/frangio68/sms_plus_plus_mcfblock_mcfsolver)
- [MILPSolver](https://gitlab.com/niccolo/milpsolver)
- [UCBlock](https://gitlab.com/AliGhezel/ucblock)
- [BundleSolver](https://gitlab.com/egorgone/bundlesolver)
- [LukFiBlock](https://gitlab.com/egorgone/lukfiblock)

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

This section is not ready yet.

## Disclaimer

The code is currently provided free of charge for academic purposes only.
As such, it is provided "*as is*", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.
