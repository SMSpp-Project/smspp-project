If you are new to SMS++ and you are looking for a zero-waste way to install
all dependencies in the default locations, we suggest using the ad-hoc
[INSTALL](#one-shot-install-scripts) files (Bash for Linux/macOS, PowerShell
for Windows).

If you instead want to install SMS++ manually, step by step (e.g. to use
existing pre-installed dependencies, customise paths, or just understand
what the scripts do), follow this guide.

[[_TOC_]]

## One-shot install scripts

### Without cloning the repository

```powershell
# Windows (from a PowerShell as administrator)
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
& ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1'))) -installRoot <your-custom-path>
```

```sh
# Linux (curl)
curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path>

# Linux (wget)
wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path>

# macOS (curl, no sudo)
curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | bash -s -- --install-root=<your-custom-path>

# macOS (wget, no sudo)
wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | bash -s -- --install-root=<your-custom-path>
```

If `<your-custom-path>` is not specified, the default installation root is
`/opt` on Linux, `/Library` on macOS, and `C:\` on Windows. On macOS the
script also falls back to `$HOME` if sudo is **not** available, the
standard output is not a TTY, or the environment variable `CI` is set
(e.g. when running in CI runners): in those cases an interactive sudo
prompt would block, so the script picks a user-writable location instead.

### Skipping individual dependencies

To exclude one or more dependencies, append the matching flags from the
table below:

| Unix               | Windows           | Description                              |
|--------------------|-------------------|------------------------------------------|
| `--without-cplex`  | `-withoutCplex`   | skip CPLEX installation                  |
| `--without-gurobi` | `-withoutGurobi`  | skip Gurobi installation                 |
| `--without-scip`   | `-withoutSCIP`    | skip SCIP installation                   |
| `--without-highs`  | *(via vcpkg)*     | skip HiGHS installation                  |
| `--without-stopt`  | *(via vcpkg)*     | skip StOpt installation                  |
| `--without-torch`  | `-withoutTorch`   | skip Torch installation                  |
| `--without-lemon`  | *(via vcpkg)*     | skip LEMON installation                  |
| `--without-coinor` | *(via vcpkg)*     | skip COIN-OR installation                |
| `--without-smspp`  | `-withoutSMSpp`   | skip SMS++ build and installation        |
| *(n/a)*            | `-updatevcpkg`    | refresh `builtin-baseline` in vcpkg.json |

> On Windows, HiGHS, StOpt and COIN-OR are pulled in unconditionally by
> vcpkg from the `vcpkg.json` manifest at configure time, so they don't
> need (and don't have) a "skip" toggle. CPLEX, Gurobi, SCIP and Torch
> are instead installed by `INSTALL.ps1` itself outside of vcpkg, hence
> the per-dependency opt-out flags above.
>
> `-updatevcpkg` rewrites the `"builtin-baseline"` field in `vcpkg.json`
> to the current `git rev-parse HEAD` of `C:\vcpkg`. Use it when your
> local vcpkg checkout has moved past the baseline pinned in the manifest
> (e.g. after a `git pull` in `C:\vcpkg`) and you want SMS++'s manifest to
> resolve packages against the new revisions.

Example:

```sh
sudo ./INSTALL.sh --install-root=<path> --without-cplex --without-gurobi
```

```powershell
.\INSTALL.ps1 -installRoot <path> -withoutCplex -withoutGurobi
```

### Inside an existing clone

```powershell
# Windows (PowerShell as administrator)
.\INSTALL.ps1
```

```sh
# Linux
sudo ./INSTALL.sh

# macOS
./INSTALL.sh
```

If you need more detail on what those scripts do — or you want to install
SMS++ manually, hand-picking the requirements — keep reading.

## Required tools

Building SMS++ requires:

- A C++17-compliant C++ compiler ([GCC] or [Clang]);
- [CMake] (suggested for SMS++, required by several requirements);
- [Git] and `make`.

Tools and requirements can be installed via common package managers:
[`apt`](https://wiki.debian.org/Apt) on Debian-based distributions,
[`vcpkg`](https://vcpkg.io/en/) on Windows,
[`homebrew`](https://brew.sh/) on macOS.

### macOS

Building on *macOS* requires Apple's Command Line Tools. Install them via
[XCode] (launch it once after install) or via terminal:

```sh
xcode-select --install
```

CLT ships [Clang], [Git], and `make`.

We suggest `homebrew` for everything else:

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install bash cmake git
```

> `INSTALL.sh` installs `bash` and `git` from homebrew even though they
> ship with the OS / CLT — newer versions are more predictable for the
> rest of the script (e.g. `bash` 4+ syntax, `git` with the latest
> protocol defaults).

> **Apple Silicon vs Intel:** SMS++ supports both. The CPLEX installer is
> different per arch (`x86-64_osx` vs `arm64_osx`); homebrew, NetCDF, Boost,
> Eigen handle the arch transparently. `INSTALL.sh` autodetects via
> `uname -m`.

### Debian / Ubuntu

Refresh the apt index first, then install the tools:

```sh
sudo apt-get update
sudo apt install build-essential clang cmake cmake-curses-gui git curl
```

`build-essential` includes [GCC] and `make`; `cmake-curses-gui` is needed
to run `ccmake` interactively. If you prefer [Clang]:

```sh
sudo update-alternatives --set cc /usr/bin/clang
sudo update-alternatives --set c++ /usr/bin/clang++
```

### Other UNIX/Linux systems

We don't actively support other distros, but you can usually adapt the
Debian/Ubuntu instructions with little effort.

### Windows

Two options:

1. **WSL** — install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
   then follow the *Debian/Ubuntu* part of this guide.

2. **Native MSVC + vcpkg + Chocolatey** — the path used by `INSTALL.ps1`:
    - install the
      [MSVC](https://visualstudio.microsoft.com/downloads/) toolchain with
      the *Desktop Development with C++* workload (must include
      `Microsoft.VisualStudio.Component.VC.Tools.x86.x64`);
    - install [Chocolatey](https://chocolatey.org/install) and then:
      ```powershell
      choco install git wget cmake -y
      ```
      (`INSTALL.ps1` does this automatically; check `ADD_CMAKE_TO_PATH=System`);
    - clone [vcpkg](https://vcpkg.io/en/getting-started.html) under `C:\vcpkg`
      and bootstrap it:
      ```powershell
      git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
      C:\vcpkg\bootstrap-vcpkg.bat
      ```
    - SMS++ uses vcpkg in **manifest mode**, so individual packages are not
      pulled by hand — they are listed in `vcpkg.json` and pulled at
      configure time. Set:
      ```powershell
      $env:VCPKG_FEATURE_FLAGS = 'manifests,registries'
      $env:VCPKG_ROOT          = 'C:\vcpkg'
      ```

## Getting started

Before installing the dependencies, decide which SMS++ modules you actually
need: different modules have different requirements, so excluding modules
you don't need can save a lot of setup time.

You can proceed in two ways:

- Use the [SMS++ project] *umbrella* and comment out the modules you don't
  need from its top-level `CMakeLists.txt`. Best if you want most modules.
- Fetch, build and install modules individually starting from the
  [SMS++ core library]. Best if you only need a few modules.

## Requirements

> **Note:** requirements are also listed in each module's `README`.
> The umbrella `CMakeLists.txt` sums them up briefly.

The [SMS++ core library] requires:

- [Boost] (headers only, minimum version 1.72)
- [NetCDF-C++]
- [Eigen]
- OpenMP (optional, but used by many submodules)

Submodules add the following (nested) requirements:

- [BundleSolver]
    - [NDOSolver/FiOracle] — included as a sub-project of BundleSolver,
      built with the umbrella by default. If installing BundleSolver
      individually, install this first. Itself requires:
        - [CoinUtils] (required by Clp and Osi);
        - [Clp], [Osi] (optional for NDOSolver/FiOracle, needed by BundleSolver);
        - [CPLEX] and/or [GUROBI] (optional Osi backends, used by BundleSolver).
    - [Torch] (the PyTorch C++ API) — optional, required by (and only
      by) BundleSolverML, which is built automatically when Torch is found.

- [MCFBlock]
    - [MCFClass] — sub-project of MCFBlock, built with the umbrella by
      default. Optionally requires [CPLEX].

- [MILPSolver] — needs at least one of:
    - [CPLEX] (minimum 12.8.0; INSTALL scripts pin **22.1.1**)
    - [GUROBI] (minimum 10.0.0; INSTALL.sh pins **13.0.1**, INSTALL.ps1 pins **12.0.1**)
    - [SCIP] (minimum 7.0.0; INSTALL scripts pin **9.2.1**, INSTALL.ps1 pins **9.0.0**)
      together with [PaPILO](https://github.com/scipopt/papilo) and TBB
    - [HiGHS] (minimum 1.5.3, or **1.14** for the HiPO interior-point
      solver, the only HiGHS solver able to handle convex QPs reliably;
      INSTALL scripts build the latest `develop` commit with
      `FAST_BUILD=ON` and `HIPO=ON`).

- [SDDPBlock]
    - [StOpt] which in turn requires
      [Boost.system], [Boost.timer], [Boost.random], [Boost.mpi] and [Eigen]
    - the [MPI] runtime corresponding to Boost.mpi (Open MPI on Linux/macOS,
      Microsoft MPI 10.1.x on Windows).

> **Note on MPI:** several SMS++ binaries (e.g. `InvestmentBlock_test`,
> `investmentblock_solver`, `sddp_solver`) transitively link
> `libboost_mpi`/`libmpi` through SDDPBlock. On systems where Open MPI / UCX
> are installed but no transport is actually usable (no IB, missing UCX
> vfs.sock, ...) `MPI_Init` may hang spinning on a futex/X11 socket.
> SMS++ ships a tiny static initialiser in `tests/common_utils.cpp` and
> `tools/common_utils.cpp` that pre-seeds `UCX_TLS=tcp,self`,
> `OMPI_MCA_btl=tcp,self`, `OMPI_MCA_pml=ob1` via `setenv(..., 0)` (no
> overwrite), so the binaries start cleanly out-of-the-box and HPC users
> who already export their own values keep full control.

The dedicated sections below cover each requirement.

### Boost

*macOS*:

```sh
brew install boost
```

*Debian/Ubuntu*:

```sh
# core library only:
sudo apt install libboost-dev

# for StOpt (SDDPBlock) also:
sudo apt install libboost-timer-dev libboost-random-dev libboost-mpi-dev
```

*Windows* (manifest mode reads `vcpkg.json`; for manual install):

```sh
vcpkg install boost --triplet x64-windows
# for StOpt:
vcpkg install boost-mpi --triplet x64-windows
```

> **Note:** if the second `vcpkg` command fails, run
> `vcpkg\downloads\msmpisetup-<x.y.z>.exe` as suggested by the error
> message (or let `INSTALL.ps1` handle MS-MPI version pinning, see
> [MS-MPI](#ms-mpi-windows-only)).

### NetCDF-C++

NetCDF-C++ wraps NetCDF-C, so both are needed.

*macOS*:

```sh
brew install hdf5 netcdf netcdf-cxx
```

*Debian/Ubuntu*:

```sh
sudo apt install libnetcdf-c++4-dev
```

*Windows*:

```sh
vcpkg install netcdf-cxx4 --triplet x64-windows
```

### Eigen

```sh
# macOS
brew install eigen

# Debian/Ubuntu
sudo apt install libeigen3-dev

# Windows
vcpkg install eigen3 --triplet x64-windows
```

### OpenMP

OpenMP is used by several solvers (SCIP via TBB+OpenMP, BundleSolver
internals, ...). Install it on Linux/macOS:

```sh
# Debian/Ubuntu
sudo apt install libomp-dev

# macOS
brew install libomp
```

On Windows the MSVC toolchain provides OpenMP natively.

### CPLEX

Install CPLEX using IBM's installer. Recommended default paths:
- macOS: `/Applications/CPLEX_Studio<ver>` (the script moves it under
  `${INSTALL_ROOT}/CPLEX_Studio`);
- Linux: `/opt/ibm/ILOG/CPLEX_Studio<ver>`;
- Windows: `C:\IBM\ILOG\CPLEX_Studio<ver>` (space-free path **strongly
  recommended** to avoid linking problems in the COIN-OR Osi
  installation phase).

> **Windows note:** the IBM installer defaults to
> `C:\Program Files\IBM\...`. `INSTALL.ps1` moves the install under
> `C:\IBM\ILOG\CPLEX_Studio` (space-free) and then walks every system
> environment variable rewriting any reference to the old path via
> `Update-EnvironmentVariables`. When installing manually, do the same
> rename + update `PATH` so that the COIN-OR Osi build and SMS++ at
> runtime can locate `cplex<ver>.dll`.
>
> **macOS note:** the CPLEX `.app` shipped inside the official zip is not
> notarized for Gatekeeper, so on Apple-silicon machines it gets a
> `com.apple.quarantine` extended attribute on download. Strip it before
> launching the installer:
> ```sh
> sudo xattr -r -d com.apple.quarantine "/path/to/cplex_studio<ver>-osx.app"
> ```
>
> **Linux note:** if CPLEX is not found at runtime, register its lib
> directory:
> ```sh
> sudo sh -c "echo '/opt/ibm/ILOG/CPLEX_Studio/cplex/lib/x86-64_linux/static_pic' > /etc/ld.so.conf.d/cplex.conf"
> sudo ldconfig
> ```
> `INSTALL.sh` writes this file automatically when run with sudo.

### GUROBI

Install GUROBI from <https://www.gurobi.com/downloads/gurobi-software/>.
Recommended default paths:
- macOS: `/Library/gurobi<ver>` (renamed to `${INSTALL_ROOT}/gurobi` by
  the script);
- Linux: `/opt/gurobi<ver>` (renamed to `/opt/gurobi`);
- Windows: `C:\gurobi<ver>` (renamed to `C:\gurobi`).

> **Linux note:** if GUROBI is not found at runtime:
> ```sh
> sudo sh -c "echo '/opt/gurobi/linux64/lib' > /etc/ld.so.conf.d/gurobi.conf"
> sudo ldconfig
> ```
> `INSTALL.sh` writes this file automatically when run with sudo.
>
> **Windows note:** as for CPLEX, `INSTALL.ps1` renames the installed
> folder to a version-less path (`C:\gurobi`) and patches every system
> environment variable that referenced the old version-suffixed one
> (`Update-EnvironmentVariables`). When installing manually, do the same.
>
> **macOS note:** if you install GUROBI somewhere other than `/Library/gurobi<ver>`,
> the dylib's install name and signature must be patched:
> ```sh
> GUROBI_HOME=<gurobi-dir>
> GUROBI_LIB_DIR=$(ls -bd1 $GUROBI_HOME/lib | tail -n1)
> GUROBI_VERSION=$(ls $GUROBI_LIB_DIR | grep -E '^libgurobi[0-9]+\.dylib$' \
>                  | sed -E 's/^libgurobi([0-9]+)\.dylib$/\1/' | head -n1)
> install_name_tool -id "$GUROBI_LIB_DIR/libgurobi$GUROBI_VERSION.dylib" \
>                       "$GUROBI_LIB_DIR/libgurobi$GUROBI_VERSION.dylib"
> codesign -s - -f "$GUROBI_LIB_DIR/libgurobi$GUROBI_VERSION.dylib"
> ```

### SCIP (and PaPILO)

Install the SCIP Optimization Suite from <https://www.scipopt.org/index.php#download>.
Recommended default paths:
- macOS: `/Library/scip`;
- Linux: `/opt/scip`;
- Windows: `C:\Program Files\SCIPOptSuite <ver>` (renamed to
  `C:\Program Files\SCIPOptSuite` by `INSTALL.ps1`).

SCIP requires the TBB threading runtime and (used internally for
presolving) [PaPILO](https://github.com/scipopt/papilo):

```sh
# Debian/Ubuntu
sudo apt install gfortran libtbb-dev

# macOS
brew install gcc tbb
```

Then either launch the official installer:

```sh
chmod u+x SCIPOptSuite-<ver>-<OS>.sh
./SCIPOptSuite-<ver>-<OS>.sh --prefix=/opt/scip --exclude-subdir --skip-license
```

or build SCIP from source with PaPILO (this is what `INSTALL.sh` does):

```sh
# build PaPILO first
git clone https://github.com/scipopt/papilo.git
cmake -S papilo -B papilo/build -DCMAKE_INSTALL_PREFIX=/opt/scip/papilo
cmake --build papilo/build -j$(nproc)
sudo cmake --install papilo/build

# then SCIP with TBB + OpenMP enabled, pointing at PaPILO
cmake -S scip-<ver> -B scip-<ver>/build \
      -DCMAKE_INSTALL_PREFIX=/opt/scip \
      -DAUTOBUILD=ON -DZIMPL=OFF \
      -DPAPILO_DIR=/opt/scip/papilo \
      -DTBB=ON -DOPENMP=ON -Wno-dev
cmake --build scip-<ver>/build -j$(nproc)
sudo cmake --install scip-<ver>/build
```

> **Linux note:** if SCIP is not found at runtime:
> ```sh
> sudo sh -c "echo '/opt/scip/lib' > /etc/ld.so.conf.d/scip.conf"
> sudo ldconfig
> ```
> `INSTALL.sh` writes this file automatically when run with sudo.
>
> **Windows note:** `INSTALL.ps1` renames `C:\Program Files\SCIPOptSuite <ver>`
> to `C:\Program Files\SCIPOptSuite` (no version suffix) and rewrites any
> environment variable that referred to the old path.

### HiGHS

Build from source. Recommended default paths:
- macOS: `/Library/HiGHS`;
- Linux: `/opt/HiGHS`;
- Windows: `C:\HiGHS`.

`HIPO=ON` builds the HiPO interior-point solver (HiGHS >= **1.14**), which
is the only HiGHS solver able to handle convex QPs reliably. It needs a
system BLAS: on Debian/Ubuntu install `libopenblas-dev`, on macOS the
Accelerate framework is used, so no extra dependency is needed.

```sh
git clone https://github.com/ERGO-Code/HiGHS.git
cd HiGHS

# Linux / macOS
sudo apt install libopenblas-dev   # Linux only, needed by HiPO
cmake -S . -B build -DFAST_BUILD=ON -DHIPO=ON -DCMAKE_INSTALL_PREFIX=/opt/HiGHS
cmake --build build -j$(nproc)
sudo cmake --install build

# Windows
cmake -S . -B build -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX=C:/HiGHS
cmake --build build --config Release
cmake --install build
```

> **Linux note:** if HiGHS is not found at runtime:
> ```sh
> sudo sh -c "echo '/opt/HiGHS/lib' > /etc/ld.so.conf.d/highs.conf"
> sudo ldconfig
> ```
> `INSTALL.sh` writes this file automatically when run with sudo.

### COIN-OR CoinUtils

`INSTALL.sh` builds CoinUtils from source via `coinbrew` (the bzip2
prerequisite goes through the system package manager):

```sh
# 1) prerequisite
sudo apt install libbz2-dev                 # Debian/Ubuntu
vcpkg install bzip2 --triplet x64-windows   # Windows
# macOS provides bzip2 via the CLT

# 2) coinbrew
curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
chmod u+x coinbrew
./coinbrew build CoinUtils --latest-release \
    --skip-dependencies --prefix=/opt/coin-or --tests=none
```

### COIN-OR Osi / Clp (with CPLEX and Gurobi)

The pre-compiled Osi packages do **not** ship CPLEX support, which is
required by BundleSolver; Clp packages depend on Osi, so they cannot be
used either. You must build Osi (and consequently Clp) against your CPLEX
and/or GUROBI install. Two options:

- `coinbrew` (recommended);
- manual build.

#### coinbrew (Linux/macOS)

```sh
# point at your CPLEX install
CPLEX_HOME=<cplex-studio-dir>/cplex
CPLEX_LIB_DIR=$(ls -bd1 $CPLEX_HOME/lib/*/static_pic | tail -n1)

# point at your GUROBI install
GUROBI_HOME=<gurobi-dir>
GUROBI_LIB_DIR=$(ls -bd1 $GUROBI_HOME/lib | tail -n1)
GUROBI_INCLUDE_DIR=$(ls -bd1 $GUROBI_HOME/*/include | tail -n1)

# Linux
GUROBI_VERSION=$(ls $GUROBI_LIB_DIR | grep -E '^libgurobi[0-9]+\.so$' \
                 | sed -E 's/^libgurobi([0-9]+)\.so$/\1/' | head -n1)
./coinbrew build Osi --latest-release --skip-dependencies \
    --prefix=/opt/coin-or --tests=none \
    --with-cplex \
    --with-cplex-lib="-L$CPLEX_LIB_DIR -lcplex -lpthread -lm" \
    --with-cplex-incdir="$CPLEX_HOME/include/ilcplex" \
    --with-gurobi \
    --with-gurobi-lib="-L$GUROBI_LIB_DIR -lgurobi$GUROBI_VERSION" \
    --with-gurobi-incdir="$GUROBI_INCLUDE_DIR"

# macOS (add --disable-cplex-libcheck / --disable-gurobi-libcheck)
GUROBI_VERSION=$(ls $GUROBI_LIB_DIR | grep -E '^libgurobi[0-9]+\.dylib$' \
                 | sed -E 's/^libgurobi([0-9]+)\.dylib$/\1/' | head -n1)
./coinbrew build Osi --latest-release --skip-dependencies \
    --prefix=/opt/coin-or --tests=none \
    --with-cplex \
    --with-cplex-lib="-L$CPLEX_LIB_DIR -lcplex -lm" --disable-cplex-libcheck \
    --with-cplex-incdir="$CPLEX_HOME/include/ilcplex" \
    --with-gurobi \
    --with-gurobi-lib="-L$GUROBI_LIB_DIR -lgurobi$GUROBI_VERSION" --disable-gurobi-libcheck \
    --with-gurobi-incdir="$GUROBI_INCLUDE_DIR"

# then Clp
./coinbrew build Clp --latest-release --skip-dependencies \
    --prefix=/opt/coin-or --tests=none
```

> **Note (macOS):** the `configure` script may not be able to validate the
> CPLEX library through the `CPXgetstat()` symbol. If the `--with-cplex-lib`
> / `--with-gurobi-lib` flags are correct, suppress the check with
> `--disable-cplex-libcheck` / `--disable-gurobi-libcheck`.

#### vcpkg (Windows)

`INSTALL.ps1` enables the CPLEX/Gurobi interfaces by generating an
**overlay port** for `coin-or-osi`, patching its `portfile.cmake` to
swap `--without-cplex`/`--without-gurobi` with the matching
`--with-cplex`/`--with-gurobi` blocks (pointing at
`C:\IBM\ILOG\CPLEX_Studio` and `C:\gurobi`), then runs `vcpkg install`
through the overlay:

```powershell
$env:VCPKG_OVERLAY_PORTS = 'C:\vcpkg\overlays\ports'
vcpkg install coin-or-osi coin-or-clp glpk pybind11 --triplet x64-windows
```

The overlay path keeps the upstream `C:\vcpkg\ports\coin-or-osi` clean
(immune to `git pull`). See the function block around `coin-or-osi` in
`INSTALL.ps1` for the exact patching logic.

### StOpt

The Stochastic Control library is built from source by `INSTALL.sh`:

```sh
# 1) prerequisites
# Debian/Ubuntu
sudo apt install zlib1g-dev libbz2-dev libboost-timer-dev libboost-random-dev libboost-mpi-dev
# macOS
brew install zlib boost-mpi
# Windows
vcpkg install zlib bzip2 --triplet x64-windows

# 2) build
git clone https://gitlab.com/stochastic-control/StOpt
cd StOpt
cmake -S . -B build -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF \
      -DCMAKE_INSTALL_PREFIX=/opt/StOpt
cmake --build build -j$(nproc)
sudo cmake --install build
```

> **Linux note:** if StOpt is not found at runtime:
> ```sh
> sudo sh -c "echo '/opt/StOpt/lib' > /etc/ld.so.conf.d/stopt.conf"
> sudo ldconfig
> ```
> `INSTALL.sh` writes this file automatically when run with sudo.

On *Windows*, StOpt is not on the public Microsoft vcpkg repository: add
the upstream vcpkg registry as an overlay-port:

```sh
git clone https://gitlab.com/stochastic-control/vcpkg-registry C:\vcpkg-registry
cd C:\vcpkg
vcpkg install stopt --overlay-ports=C:\vcpkg-registry\ports\stopt --triplet x64-windows
```

### Torch

[Torch] (the PyTorch C++ API) is required by (and only by)
BundleSolverML, which BundleSolver builds automatically when Torch is
found. The INSTALL scripts pin the **2.5.1** CPU distribution.
Recommended default paths:
- macOS: `/Library/libtorch`;
- Linux: `/opt/libtorch`;
- Windows: `C:\libtorch`.

```sh
# Linux
curl -O https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip
sudo unzip -q libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip -d /opt

# macOS (Intel)
curl -O https://download.pytorch.org/libtorch/cpu/libtorch-macos-x86_64-2.5.1.zip
unzip -q libtorch-macos-x86_64-2.5.1.zip -d /Library

# macOS (Apple Silicon)
curl -O https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-2.5.1.zip
unzip -q libtorch-macos-arm64-2.5.1.zip -d /Library
```

```powershell
# Windows
Invoke-WebRequest -Uri "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-2.5.1%2Bcpu.zip" -OutFile "C:\libtorch.zip"
Expand-Archive -Path "C:\libtorch.zip" -DestinationPath "C:\" -Force
Remove-Item "C:\libtorch.zip"
```

> **Linux note:** if Torch is not found at runtime:
> ```sh
> sudo sh -c "echo '/opt/libtorch/lib' > /etc/ld.so.conf.d/libtorch.conf"
> sudo ldconfig
> ```
> `INSTALL.sh` writes this file automatically when run with sudo.
>
> **Windows note:** `INSTALL.ps1` adds `C:\libtorch\lib` to the system
> `PATH` so that the SMS++ executables can locate the `torch*.dll` files.
> When installing manually, do the same.

### MS-MPI (Windows only)

SDDPBlock+StOpt require an MPI runtime. On Windows that's
[Microsoft MPI](https://www.microsoft.com/en-us/download/details.aspx?id=100593).
`INSTALL.ps1` pins it to **10.1.12498.52** and uninstalls any other
version automatically (see `Ensure-MsMpiVersion`).

Manually:

```powershell
# pick up the installer that vcpkg/boost-mpi already downloaded
$installer = "C:\vcpkg\downloads\msmpisetup-10.1.12498.52.exe"
Start-Process -FilePath $installer -ArgumentList "-unattend","-force" -Wait
```

## SMS++

Two paths, depending on how many modules you need:

- use the **umbrella** to fetch multiple modules at once;
- fetch, build and install modules **individually**.

### Using the umbrella

1. Clone the umbrella and (optionally) its submodules:

   ```sh
   # everything in one go
   git clone -b develop --recurse-submodules https://gitlab.com/smspp/smspp-project.git
   cd smspp-project
   git submodule sync --recursive
   git submodule update --init --recursive

   # or pick the modules you need
   git clone -b develop https://gitlab.com/smspp/smspp-project.git
   cd smspp-project
   git submodule update --init SMS++ UCBlock tools   # etc.
   ```

   If the umbrella is already on disk, refresh it with:

   ```sh
   git pull --recurse-submodules
   git submodule sync --recursive
   git submodule update --init --recursive
   ```

   > **Note:** SMS++ is developed rapidly, the `develop` branch is the
   > canonical entry point. The `master` branch lags significantly behind.
   > `git submodule sync --recursive` is recommended after every pull
   > because submodule URLs may have moved.

2. *Optional:* edit the top-level `CMakeLists.txt` and comment out
   modules you don't need (e.g. `BUILD_SDDPBlock`, `BUILD_BundleSolver`,
   ...).

3. *Optional, makefile-based builds:* if you installed the requirements
   to non-default paths, override them in `extlib/makefile-paths` (this
   file is `.gitignore`-d). Same for `BundleSolver/NdoFiOracle/extlib/makefile-paths`
   and `MCFBlock/MCFClass/extlib/makefile-paths`. `INSTALL.sh` generates
   these automatically when `--install-root` is non-default. Example:

   ```make
   CPLEX_ROOT     = /opt/ibm/ILOG/CPLEX_Studio
   GUROBI_ROOT    = /opt/gurobi
   SCIP_ROOT      = /opt/scip
   HiGHS_ROOT     = /opt/HiGHS
   StOpt_ROOT     = /opt/StOpt
   CoinUtils_ROOT = /opt/coin-or
   Osi_ROOT       = /opt/coin-or
   Clp_ROOT       = /opt/coin-or
   Torch_ROOT     = /opt/libtorch
   ```

4. Configure with CMake:

   ```sh
   # Linux / macOS
   cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$SMSPP_ROOT" -Wno-dev

   # Windows (manifest-mode vcpkg)
   cmake -S . -B build -G "Visual Studio 17 2022" `
         -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
         -DCMAKE_INSTALL_PREFIX=C:\smspp-project -Wno-dev
   ```

   `$SMSPP_ROOT` is the directory you want SMS++ installed to
   (e.g. `/opt/smspp-project` on Linux, `/Library/smspp-project` or
   `$HOME/smspp-project` on macOS). `-Wno-dev` silences CMake warnings
   that are meant for project developers, not users.

   For more options see [Customize-the-configuration](Customize-the-configuration).

5. If you want to enable/disable individual `BUILD_*` flags interactively,
   run `ccmake` (Linux/macOS) or `cmake-gui` (Windows) on the build dir:

   ```sh
   cd build
   ccmake ..        # Linux/macOS
   cmake-gui .      # Windows
   ```

   `INSTALL.sh`/`INSTALL.ps1` do this automatically if a TTY is attached
   and `cmake-gui` is available.

6. Build and install:

   ```sh
   # Linux / macOS
   cmake --build build -j$(nproc)
   sudo cmake --install build         # optional

   # Windows
   cmake --build build --config Release -j $env:NUMBER_OF_PROCESSORS
   cmake --install build --config Release
   ```

Alternatively you can build with the makefiles bundled with each module.
See *Build and install with makefiles* in the umbrella `README`.

### Build modules individually

Repeat the following for each module (example: the [SMS++ core library]):

```sh
git clone -b develop https://gitlab.com/smspp/smspp.git
cd smspp
cmake -S . -B build
cmake --build build -j$(nproc)
sudo cmake --install build           # required for other modules to find it
```

> **Note:** the install step is required so other modules can locate the
> library through `find_package`. The alternative is CMake's
> [User Package Registry](Customize-the-configuration).

> **Parallel jobs:** the snippets above use `-j$(nproc)`, which only works
> on Linux. `INSTALL.sh` autodetects the number of jobs across platforms:
> ```sh
> if command -v nproc >/dev/null 2>&1; then       # Linux
>   MAX_JOBS=$(nproc)
> elif [ "$(uname)" = "Darwin" ]; then            # macOS
>   MAX_JOBS=$(sysctl -n hw.ncpu)
> else
>   MAX_JOBS=1
> fi
> cmake --build build -j "$MAX_JOBS"
> ```
> On Windows, `$env:NUMBER_OF_PROCESSORS` plays the same role.

## Post-install actions

Once SMS++ and its requirements are in place, the installer scripts also
take care of a few "make it work everywhere" steps. If you installed
things manually, replicate them or your shell / runtime loader will not
find the binaries and shared libraries.

### Updating the dynamic linker (Linux only)

When run as root, `INSTALL.sh` registers each requirement's lib directory
under `/etc/ld.so.conf.d/<name>.conf` and reloads the cache:

```sh
sudo sh -c "echo '${CPLEX_HOME}/lib/x86-64_linux/static_pic' > /etc/ld.so.conf.d/cplex.conf"
sudo sh -c "echo '${GUROBI_HOME}/lib'                        > /etc/ld.so.conf.d/gurobi.conf"
sudo sh -c "echo '${SCIP_ROOT}/lib'                          > /etc/ld.so.conf.d/scip.conf"
sudo sh -c "echo '${HiGHS_ROOT}/lib'                         > /etc/ld.so.conf.d/highs.conf"
sudo sh -c "echo '${CoinOr_ROOT}/lib'                        > /etc/ld.so.conf.d/coin-or.conf"
sudo sh -c "echo '${StOpt_ROOT}/lib'                         > /etc/ld.so.conf.d/stopt.conf"
sudo sh -c "echo '${Torch_ROOT}/lib'                         > /etc/ld.so.conf.d/libtorch.conf"
sudo sh -c "echo '${SMSPP_ROOT}/lib'                         > /etc/ld.so.conf.d/smspp.conf"
sudo ldconfig
```

If you are installing without sudo (e.g. `INSTALL_ROOT=$HOME`), skip this
step and use the per-shell `LD_LIBRARY_PATH` updates of the next section
instead.

### Per-shell PATH / LD_LIBRARY_PATH (Linux / macOS)

`INSTALL.sh` appends two lines to the *current user's* shell rc file
(`~/.zshrc` if `$SHELL` ends in `zsh`, `~/.bashrc` otherwise) so that
new shells pick up the SMS++ executables and shared libs:

```sh
# Linux
echo "export PATH=\"\$SMSPP_ROOT/bin:\$PATH\""                       >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\"\$SMSPP_ROOT/lib:\$LD_LIBRARY_PATH\"" >> ~/.bashrc

# macOS (Mach-O loader uses DYLD_LIBRARY_PATH, not LD_LIBRARY_PATH)
echo "export PATH=\"\$SMSPP_ROOT/bin:\$PATH\""                            >> ~/.zshrc
echo "export DYLD_LIBRARY_PATH=\"\$SMSPP_ROOT/lib:\$DYLD_LIBRARY_PATH\""  >> ~/.zshrc
```

The script also wipes any older `SMSPP_BIN` / `SMSPP_LIB` line in the
same file before appending the new ones, so re-running the installer
against a different `--install-root` cleanly updates the environment.

To activate the change in the **current** shell, either re-`source` the
rc file or open a new shell:

```sh
source ~/.bashrc   # or ~/.zshrc
```

### System PATH and env vars (Windows)

`INSTALL.ps1` uses two helpers:

- `Add-ToSystemPath` adds `$SMSPP_ROOT\bin` and the vcpkg `bin` folder
  (`$SMSPP_ROOT\build\vcpkg_installed\$TRIPLET\bin`) to the system `PATH`
  (HKLM), idempotently;
- `Update-EnvironmentVariables` walks every machine-scope env var and
  rewrites any reference to the original installer paths
  (`C:\Program Files\IBM\...`, `C:\gurobi<ver>`, `C:\Program Files\SCIPOptSuite <ver>`)
  with the version-less destination paths (`C:\IBM\...`, `C:\gurobi`,
  `C:\Program Files\SCIPOptSuite`).

To refresh the current PowerShell session without logging out, the
script calls `refreshenv` (provided by Chocolatey). When installing
manually:

```powershell
refreshenv          # or: open a new PowerShell
```

### MPI / UCX safe defaults

Several SMS++ binaries (anything that pulls in `SDDPBlock` transitively:
`InvestmentBlock_test`, `investmentblock_solver`, `sddp_solver`, ...) link
`libboost_mpi`/`libmpi` even when they don't actually call `MPI_Init`.
On a fresh Linux/macOS install where Open MPI / UCX are present but no
real transport is available, that can cause the binary to hang at
startup spinning on futex / X11 sockets.

SMS++ ships a static initializer (in `tests/common_utils.cpp` and
`tools/common_utils.cpp`) that pre-seeds three safe defaults before
`main()`:

```sh
UCX_TLS=tcp,self
OMPI_MCA_btl=tcp,self
OMPI_MCA_pml=ob1
```

They use `setenv(name, value, 0)`, i.e. the third argument is "do not
overwrite": HPC users who already export their own `UCX_TLS=mlx5_0:1` or
similar keep full control. **You don't need to do anything**, but if a
binary should ever still hang on startup, exporting the three variables
manually is a safe override:

```sh
export UCX_TLS=tcp,self OMPI_MCA_btl=tcp,self OMPI_MCA_pml=ob1
```

## The test environment

A reference setup of SMS++ and all its requirements is provided in our
[Test Environment]. You can either use it as-is or read its scripts for
inspiration — `smsbuild` can drive either the umbrella or the individual
modules and offers several knobs.

## Issues and troubleshooting

For support on a single module, see the *Getting help* section of its
`README`.

For project-level installation issues not covered by the
[troubleshooting page](Troubleshooting), or to propose a new module,
[open a new issue](https://gitlab.com/smspp/smspp-project/-/issues/new).


[SMS++ project]:       https://gitlab.com/smspp/smspp-project
[SMS++ core library]:  https://gitlab.com/smspp/smspp
[BundleSolver]:        https://gitlab.com/smspp/bundlesolver
[MCFBlock]:            https://gitlab.com/smspp/mcfblock
[SDDPBlock]:           https://gitlab.com/smspp/sddpblock
[MILPSolver]:          https://gitlab.com/smspp/milpsolver
[Test Environment]:    https://gitlab.com/smspp/test-env
[MCFClass]:            https://github.com/frangio68/Min-Cost-Flow-Class

[NDOSolver/FiOracle]:  https://gitlab.com/frangio68/ndosolver_fioracle_project
[coinbrew]:            https://coin-or.github.io/coinbrew/
[Osi]:                 https://github.com/coin-or/Osi
[Clp]:                 https://github.com/coin-or/Clp
[CoinUtils]:           https://github.com/coin-or/CoinUtils
[StOpt]:               https://gitlab.com/stochastic-control/StOpt

[Boost]:               https://www.boost.org/
[Boost.system]:        https://www.boost.org/doc/libs/release/libs/system/
[Boost.timer]:         https://www.boost.org/doc/libs/release/libs/timer/
[Boost.random]:        https://www.boost.org/doc/libs/release/libs/random/
[Boost.mpi]:           https://www.boost.org/doc/libs/release/libs/mpi/
[MPI]:                 https://www.mpi-forum.org/
[NetCDF-C++]:          https://www.unidata.ucar.edu/software/netcdf
[Eigen]:               http://eigen.tuxfamily.org
[CPLEX]:               https://www.ibm.com/products/ilog-cplex-optimization-studio
[GUROBI]:              https://www.gurobi.com/
[SCIP]:                https://scipopt.org/index.php
[HiGHS]:               https://highs.dev/
[Torch]:               https://pytorch.org/get-started/locally/

[GCC]:                 https://gcc.gnu.org/
[Clang]:               https://clang.llvm.org/
[CMake]:               https://cmake.org/
[Git]:                 https://git-scm.com/
[XCode]:               https://apps.apple.com/it/app/xcode/id497799835
