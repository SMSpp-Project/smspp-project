#!/bin/bash

# ------------------------------------------------------------------------------
# SYNOPSIS
#     This script installs SMS++ and all its dependencies on Unix-based systems.
#
# DESCRIPTION
#     This script performs the installation of SMS++ and all its dependencies
#     on Unix-based systems. If not already present, it clones the smspp-project
#     repositories, then builds and installs them.
#
#     You can use the `--install-root=<your-custom-path>` option to specify your custom installation root.
#     You can use the `--cplex-installer=<path-to-installer>` option to point to your own CPLEX installer.
#     You can use the `--without-cplex` option to skip the installation of CPLEX.
#     You can use the `--without-gurobi` option to skip the installation of Gurobi.
#     You can use the `--without-scip` option to skip the installation of SCIP.
#     You can use the `--without-highs` option to skip the installation of HiGHS.
#     You can use the `--without-pips` option to skip the installation of PIPS-IPM++.
#     You can use the `--without-stopt` option to skip the installation of StOpt.
#     You can use the `--without-torch` option to skip the installation of Torch.
#     You can use the `--without-lemon` option to skip the installation of LEMON.
#     You can use the `--without-coinor` option to skip the installation of COIN-OR.
#     You can use the `--without-smspp` option to skip the installation of SMS++.
#
#     Skipping a dependency that an SMS++ module hard-requires automatically
#     disables that module when building SMS++, so configuration does not fail
#     looking for a missing library: --without-stopt disables SDDPBlock and
#     InvestmentBlock, --without-lemon disables MCFLemonSolver,
#     --without-coinor disables BundleSolver, and --without-pips disables
#     PIPSMILPSolver.
#
# AUTHOR
#     Donato Meoli
#
# EXAMPLES
#     If you are inside the cloned repository:
#
#         sudo ./INSTALL.sh --install-root=<your-custom-path> --without-<some-dependency>
#
#     If you have not yet cloned the SMS++ repository, you can run the script directly:
#
#     Using `curl`:
#
#         If you want to install SMS++ with all dependencies:
#
#             curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path> --without-<some-dependency>
#
#     Using `wget`:
#
#         If you want to install SMS++ with all dependencies:
#
#             wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash -s -- --install-root=<your-custom-path> --without-<some-dependency>
#
#     Notice that on macOS all the previous commands should be run **without** the `sudo`.
#
# ------------------------------------------------------------------------------

cleanup_on_error() {
  if [ -n "$CURRENT_INSTALL_FOLDER" ]; then
    rm -Rf "$CURRENT_INSTALL_FOLDER"
  fi
  exit 1
}

# Resolve where a dependency lives (Linux only): reuse an existing install under
# the default system root (DEFAULT_ROOT) if present, otherwise install it under
# the writable INSTALL_ROOT. With an explicit --install-root the two roots
# coincide, so no default-location probing happens and it is placed there.
resolve_dep_root() {
  if [ -d "${DEFAULT_ROOT}/$1" ]; then
    echo "${DEFAULT_ROOT}/$1"
  else
    echo "${INSTALL_ROOT}/$1"
  fi
}

# Extract a .zip archive ($1) into a destination directory ($2). unzip is not
# installed on every system (and cannot be apt-installed without sudo) and GNU
# tar cannot read zip, so fall back to python3, which is always available.
extract_zip() {
  if command -v unzip >/dev/null 2>&1; then
    unzip -q "$1" -d "$2"
  else
    python3 -m zipfile -e "$1" "$2"
  fi
}

# Function to install dependencies on Linux
install_on_linux() {
  set -e  # Exit immediately if a command exits with a non-zero status
  trap 'cleanup_on_error' ERR

  echo "Starting the installation process on Linux..."

  if [ "$HAS_SUDO" -eq 1 ]; then
    # Update packages and install basic requirements
    echo "Updating system and installing basic requirements..."
    apt-get update -q
    apt-get install -y -q build-essential clang cmake cmake-curses-gui git curl

    # Install Boost libraries
    echo "Installing Boost libraries..."
    apt-get install -y -q libboost-dev

    # Install OpenMP
    echo "Installing OpenMP..."
    apt-get install -y -q libomp-dev

    # Install Eigen
    echo "Installing Eigen..."
    apt-get install -y -q libeigen3-dev

    # Install NetCDF-C++
    echo "Installing NetCDF-C++..."
    apt-get install -y -q libnetcdf-c++4-dev

    # Install LEMON
    if [ "$install_lemon" -eq 1 ]; then
      echo "Installing LEMON..."
      apt-get install -y -q liblemon-dev
    fi
  fi

  # Install CPLEX
  if [ "$install_cplex" -eq 1 ]; then
    echo "Installing CPLEX..."
    CPLEX_ROOT="$(resolve_dep_root ibm/ILOG/CPLEX_Studio)"
    CURRENT_INSTALL_FOLDER="${INSTALL_ROOT}/ibm"
    if [ ! -d "$CPLEX_ROOT" ]; then
      cd "$INSTALL_ROOT"
      if [ -z "$cplex_installer" ] || [ ! -f "$cplex_installer" ]; then
        echo "CPLEX installer not provided. Download IBM ILOG CPLEX Optimization"
        echo "Studio yourself (e.g. through the IBM Academic Initiative) and either"
        echo "install it manually or re-run with --cplex-installer=<path-to-installer>."
        echo "Skipping CPLEX."
        install_cplex=0
      else
        CPLEX_INSTALLER="$cplex_installer"
        chmod u+x "$CPLEX_INSTALLER"
        cat <<EOL > installer.properties
INSTALLER_UI=silent
LICENSE_ACCEPTED=TRUE
USER_INSTALL_DIR=$CPLEX_ROOT
EOL
        "$CPLEX_INSTALLER" -f ./installer.properties &
        wait $! # wait for CPLEX installer to finish
        INSTALLER_EXIT_CODE=$?
        if [ $INSTALLER_EXIT_CODE -eq 0 ]; then
          rm installer.properties
          export CPLEX_HOME="${CPLEX_ROOT}/cplex"
          export PATH="${PATH}:${CPLEX_HOME}/bin/x86-64_linux"
          export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CPLEX_HOME}/lib/x86-64_linux/static_pic"
          if [ "$HAS_SUDO" -eq 1 ]; then
            sh -c "echo '${CPLEX_HOME}/lib/x86-64_linux/static_pic' > /etc/ld.so.conf.d/cplex.conf"
            ldconfig
          else
            rm -R javasharedresources
          fi
        else
          echo "CPLEX installation failed with exit code $INSTALLER_EXIT_CODE."
          exit 1
        fi
      fi
    else
      echo "CPLEX already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install Gurobi
  if [ "$install_gurobi" -eq 1 ]; then
    echo "Installing Gurobi..."
    GUROBI_ROOT="$(resolve_dep_root gurobi)"
    CURRENT_INSTALL_FOLDER=${GUROBI_ROOT}
    if [ ! -d "$GUROBI_ROOT" ]; then
      cd "$INSTALL_ROOT"
      GUROBI_INSTALLER="gurobi13.0.1_linux64.tar.gz"
      curl -O "https://packages.gurobi.com/13.0/$GUROBI_INSTALLER"
      tar -xzf "$GUROBI_INSTALLER"
      rm "$GUROBI_INSTALLER"
      mv ./gurobi1301 "$GUROBI_ROOT"
      export GUROBI_HOME="${GUROBI_ROOT}/linux64"
      export PATH="${PATH}:${GUROBI_HOME}/bin"
      export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
      if [ "$HAS_SUDO" -eq 1 ]; then
        sh -c "echo '${GUROBI_HOME}/lib' > /etc/ld.so.conf.d/gurobi.conf"
        ldconfig
      fi
    else
      echo "Gurobi already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install SCIP
  if [ "$install_scip" -eq 1 ]; then
    echo "Installing SCIP..."
    SCIP_ROOT="$(resolve_dep_root scip)"
    CURRENT_INSTALL_FOLDER=${SCIP_ROOT}
    if [ ! -d "$SCIP_ROOT" ]; then
      if [ "$HAS_SUDO" -eq 1 ]; then
        apt-get install -y -q gfortran libtbb-dev
      fi
      cd "$INSTALL_ROOT"
      SCIP_INSTALLER="scip-9.2.1"
      curl -O "https://www.scipopt.org/download/release/$SCIP_INSTALLER.tgz"
      tar -xzf "$SCIP_INSTALLER.tgz"
      rm "$SCIP_INSTALLER.tgz"
      mv ./"$SCIP_INSTALLER" "$SCIP_ROOT"
      cd "$SCIP_ROOT"
      # Install PaPILO
      git clone https://github.com/scipopt/papilo.git
      cd papilo
      cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${SCIP_ROOT}/papilo"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$SCIP_ROOT"
      scip_build_flags=(
        "-DCMAKE_INSTALL_PREFIX=${SCIP_ROOT}"
        "-DAUTOBUILD=ON"
        "-DZIMPL=OFF"
        "-DPAPILO_DIR=${SCIP_ROOT}/papilo"
        "-DTBB=ON"
        "-DOPENMP=ON"
        "-Wno-dev"
      )
      cmake -S . -B build "${scip_build_flags[@]}"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$INSTALL_ROOT"
      if [ "$HAS_SUDO" -eq 1 ]; then
        sh -c "echo '${SCIP_ROOT}/lib' > /etc/ld.so.conf.d/scip.conf"
        ldconfig
      fi
    else
      echo "SCIP already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install HiGHS
  # HIPO=ON builds the HiPO interior point solver (HiGHS >= 1.14), which is
  # the only HiGHS solver able to handle convex QPs reliably; it needs a
  # system BLAS.
  if [ "$install_highs" -eq 1 ]; then
    echo "Installing HiGHS..."
    HiGHS_ROOT="$(resolve_dep_root HiGHS)"
    CURRENT_INSTALL_FOLDER=${HiGHS_ROOT}
    if [ "$HAS_SUDO" -eq 1 ]; then
      apt-get install -y -q libopenblas-dev
    fi
    if [ ! -d "$HiGHS_ROOT" ]; then
      cd "$INSTALL_ROOT"
      git clone https://github.com/ERGO-Code/HiGHS.git
      cd HiGHS
      # BLA_VENDOR routes CMake to the system libopenblas: Ubuntu's
      # OpenBLASConfig.cmake advertises a lib/libopenblas.so path that
      # does not exist
      cmake -S . -B build -DFAST_BUILD=ON -DHIPO=ON -DBLA_VENDOR=OpenBLAS -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      if [ "$HAS_SUDO" -eq 1 ]; then
        sh -c "echo '${HiGHS_ROOT}/lib' > /etc/ld.so.conf.d/highs.conf"
        ldconfig
      fi
    else
      if [ "$HAS_SUDO" -eq 1 ]; then
        cd "$HiGHS_ROOT"
        git remote update
        LOCAL=$(git rev-parse @)
        REMOTE=$(git rev-parse @{u})
        # if the repository is not up to date
        if [ "$LOCAL" != "$REMOTE" ]; then
          git pull
          cmake -S . -B build -DFAST_BUILD=ON -DHIPO=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
          cmake --build build -j "${MAX_JOBS}"
          cmake --install build
        else
          echo "HiGHS already up to date."
        fi
      fi
    fi
    cd "$INSTALL_ROOT"
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install PIPS-IPM++
  # PIPS-IPM++ is consumed from its build tree (no install step); the fully
  # open-source build uses MUMPS as inner linear solver.
  if [ "$install_pips" -eq 1 ]; then
    echo "Installing PIPS-IPM++..."
    PIPS_ROOT_DIR="$(resolve_dep_root pips-ipmpp)"
    CURRENT_INSTALL_FOLDER=${PIPS_ROOT_DIR}
    if [ ! -d "$PIPS_ROOT_DIR" ]; then
      if [ "$HAS_SUDO" -eq 1 ]; then
        apt-get install -y -q libopenmpi-dev gfortran liblapack-dev libmetis-dev libmumps-dev
      fi
      cd "$INSTALL_ROOT"
      git clone --recurse-submodules https://gitlab.com/pips-ipmpp/pips-ipmpp.git "$PIPS_ROOT_DIR"
      cmake -S "$PIPS_ROOT_DIR" -B "$PIPS_ROOT_DIR/build" -DCMAKE_BUILD_TYPE=Release
      cmake --build "$PIPS_ROOT_DIR/build" -j "${MAX_JOBS}"
    else
      echo "PIPS-IPM++ already installed."
    fi
    cd "$INSTALL_ROOT"
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install COIN-OR CoinUtils and Osi/Clp
  if [ "$install_coinor" -eq 1 ]; then
    echo "Installing COIN-OR CoinUtils and Osi/Clp..."
    CoinOr_ROOT="$(resolve_dep_root coin-or)"
    CURRENT_INSTALL_FOLDER=${CoinOr_ROOT}
    if [ "$HAS_SUDO" -eq 1 ]; then
      apt-get install -y -q libbz2-dev
    fi
    if [ ! -d "$CoinOr_ROOT" ]; then
      cd "$INSTALL_ROOT"
      curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
      chmod u+x coinbrew
      # Build CoinUtils
      ./coinbrew build CoinUtils --latest-release --skip-dependencies --prefix="$CoinOr_ROOT" --tests=none
      osi_build_flags=(
        "--latest-release"
        "--skip-dependencies"
        "--prefix=$CoinOr_ROOT"
        "--tests=none"
      )
      # Build Osi with or without CPLEX
      if [ "$install_cplex" -eq 0 ]; then
        osi_build_flags+=("--without-cplex")
      else
        osi_build_flags+=(
          "--with-cplex"
          "--with-cplex-lib=-L${CPLEX_ROOT}/cplex/lib/x86-64_linux/static_pic -lcplex -lpthread -lm"
          "--disable-cplex-libcheck"
          "--with-cplex-incdir=${CPLEX_ROOT}/cplex/include/ilcplex"
        )
      fi
      # Build Osi with or without Gurobi
      if [ "$install_gurobi" -eq 0 ]; then
        osi_build_flags+=("--without-gurobi")
      else
        GUROBI_VERSION=$(ls "${GUROBI_ROOT}/linux64/lib" | grep -E '^libgurobi[0-9]+\.so$' | sed -E 's/^libgurobi([0-9]+)\.so$/\1/' | head -n1)
        osi_build_flags+=(
          "--with-gurobi"
          "--with-gurobi-lib=-L${GUROBI_ROOT}/linux64/lib -lgurobi${GUROBI_VERSION}"
          "--disable-gurobi-libcheck"
          "--with-gurobi-incdir=${GUROBI_ROOT}/linux64/include"
        )
      fi
      ./coinbrew build Osi "${osi_build_flags[@]}"
      # Build Clp
      ./coinbrew build Clp --latest-release --skip-dependencies --prefix="$CoinOr_ROOT" --tests=none
      rm -Rf coinbrew build CoinUtils Osi Clp
      export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CoinOr_ROOT}/lib"
      if [ "$HAS_SUDO" -eq 1 ]; then
        sh -c "echo '${CoinOr_ROOT}/lib' > /etc/ld.so.conf.d/coin-or.conf"
        ldconfig
      fi
    else
      echo "COIN-OR already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install StOpt
  if [ "$install_stopt" -eq 1 ]; then
    echo "Installing StOpt..."
    StOpt_ROOT="$(resolve_dep_root StOpt)"
    CURRENT_INSTALL_FOLDER=${StOpt_ROOT}
    if [ "$HAS_SUDO" -eq 1 ]; then
      apt-get install -y -q zlib1g-dev libboost-timer-dev libboost-random-dev libboost-mpi-dev
    fi
    if [ ! -d "$StOpt_ROOT" ]; then
      cd "$INSTALL_ROOT"
      git clone https://gitlab.com/stochastic-control/StOpt.git
      cd StOpt
      cmake -S . -B build \
            -DBUILD_PYTHON=OFF \
            -DBUILD_TEST=OFF \
            -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$INSTALL_ROOT"
      if [ "$HAS_SUDO" -eq 1 ]; then
        sh -c "echo '${StOpt_ROOT}/lib' > /etc/ld.so.conf.d/stopt.conf"
        ldconfig
      fi
    else
      if [ "$HAS_SUDO" -eq 1 ]; then
        cd "$StOpt_ROOT"
        LOCAL=$(git rev-parse @)
        REMOTE=$(git rev-parse @{u})
        # if the repository is not up to date
        if [ "$LOCAL" != "$REMOTE" ]; then
          git pull
          cmake -S . -B build \
                -DBUILD_PYTHON=OFF \
                -DBUILD_TEST=OFF \
                -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
          cmake --build build -j "${MAX_JOBS}"
          cmake --install build
        else
          echo "StOpt already up to date."
        fi
        cd "$INSTALL_ROOT"
      fi
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install Torch
  if [ "$install_torch" -eq 1 ]; then
    echo "Installing Torch..."
    Torch_ROOT="$(resolve_dep_root torch)"
    CURRENT_INSTALL_FOLDER=${Torch_ROOT}
    if [ ! -d "$Torch_ROOT" ]; then
      cd "$INSTALL_ROOT"
      TORCH_VERSION="2.12.0"
      TORCH_INSTALLER="libtorch-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip"
      # -f fails on HTTP errors (no error page saved as a fake zip),
      # -L follows download.pytorch.org's redirect
      curl -fL -O "https://download.pytorch.org/libtorch/cpu/$TORCH_INSTALLER"
      extract_zip "$TORCH_INSTALLER" .
      rm "$TORCH_INSTALLER"
      # the archive unpacks as libtorch, rename to the version-less torch
      mv ./libtorch "$Torch_ROOT"
      export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${Torch_ROOT}/lib"
      if [ "$HAS_SUDO" -eq 1 ]; then
        sh -c "echo '${Torch_ROOT}/lib' > /etc/ld.so.conf.d/torch.conf"
        ldconfig
      fi
    else
      echo "Torch already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  echo "Installation completed successfully on Linux."
}

# Function to install dependencies on macOS
install_on_macos() {
  set -e  # Exit immediately if a command exits with a non-zero status
  trap 'cleanup_on_error' ERR

  echo "Starting the installation process on macOS..."

  # Store arch details
  if [ "$(uname -m)" == "x86_64" ]; then # Intel arch
    OSX_ARCH="x86-64_osx"
  else # Apple Silicon MX arch
    OSX_ARCH="arm64_osx"
  fi

  # Install Homebrew
  if command -v brew >/dev/null 2>&1; then
    echo "Homebrew already installed."
  else
    echo "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  fi

  # Install Xcode Command Line Tools (includes build-essential and clang)
  if xcode-select -p >/dev/null 2>&1; then
    echo "Xcode Command Line Tools already installed."
  else
    echo "Installing Xcode Command Line Tools..."
    xcode-select --install
  fi

  # Install basic requirements
  echo "Installing basic requirements..."
  brew install bash cmake git

  # Install OpenMP
  echo "Installing OpenMP..."
  brew install libomp

  # Install Boost libraries
  echo "Installing Boost libraries..."
  brew install boost

  # Install Eigen
  echo "Installing Eigen..."
  brew install eigen

  # Install NetCDF
  echo "Installing NetCDF..."
  brew install hdf5 netcdf netcdf-cxx

  # Install LEMON
  # NOTE: Homebrew core has no graph-LEMON ("brew install lemon" is the LALR
  # parser generator) and the community taps are unmaintained, so we use the
  # MacPorts port "coinor-liblemon". MacPorts is bootstrapped from source only
  # when genuinely absent (Xcode Command Line Tools, installed above, are its
  # only prereq); when LEMON is already present the whole step is skipped, so
  # no recompilation and no sudo prompt on repeated runs.
  if [ "$install_lemon" -eq 1 ]; then
    echo "Installing LEMON..."
    # Surface an already-installed MacPorts even if /opt/local/bin is off PATH
    # (otherwise it would be re-bootstrapped from source on every run).
    if ! command -v port >/dev/null 2>&1 && [ -x /opt/local/bin/port ]; then
      export PATH="/opt/local/bin:/opt/local/sbin:${PATH}"
    fi
    if [ -d /opt/local/include/lemon ] || [ -d /usr/local/include/lemon ]; then
      echo "LEMON already installed; skipping."
    else
      # Bootstrap MacPorts from source only when it is genuinely absent.
      if ! command -v port >/dev/null 2>&1; then
        echo "MacPorts not found. Bootstrapping from source..."
        MACPORTS_SRC="${INSTALL_ROOT}/macports-base"
        if [ ! -d "$MACPORTS_SRC" ]; then
          git clone --depth 1 https://github.com/macports/macports-base.git "$MACPORTS_SRC"
        fi
        cd "$MACPORTS_SRC"
        ./configure
        make
        sudo make install
        # MacPorts installs into /opt/local; make port visible for the rest of run
        export PATH="/opt/local/bin:/opt/local/sbin:${PATH}"
        sudo port -v selfupdate
      fi
      sudo port install coinor-liblemon
    fi
  fi

  # Install CPLEX
  if [ "$install_cplex" -eq 1 ]; then
    echo "Installing CPLEX..."
    CPLEX_ROOT="${INSTALL_ROOT}/CPLEX_Studio"
    CURRENT_INSTALL_FOLDER=${CPLEX_ROOT}
    if [ ! -d "$CPLEX_ROOT" ]; then
      cd "$INSTALL_ROOT"
      if [ -z "$cplex_installer" ] || [ ! -f "$cplex_installer" ]; then
        echo "CPLEX installer not provided. Download IBM ILOG CPLEX Optimization"
        echo "Studio yourself (e.g. through the IBM Academic Initiative) and either"
        echo "install it manually or re-run with --cplex-installer=<path-to-installer>."
        echo "Skipping CPLEX."
        install_cplex=0
      else
        # Create a temporary directory to extract the files
        TEMP_DIR="/tmp/cplex_install"
        mkdir -p "$TEMP_DIR"
        # Extract directly into the temporary directory
        sudo unzip "$cplex_installer" -d "$TEMP_DIR"
        # Locate the extracted installer app, whose name embeds the CPLEX version
        CPLEX_APP=$(find "$TEMP_DIR" -maxdepth 1 -name '*.app' | head -n 1)
        CPLEX_NAME=$(basename "${CPLEX_APP%.app}")
        # Remove macOS quarantine attribute to allow execution of unsigned app
        sudo xattr -r -d com.apple.quarantine "$CPLEX_APP"
        # Launch the installer
        sudo "${CPLEX_APP}/Contents/MacOS/${CPLEX_NAME}" &
        wait $! # wait for the installer to finish
        INSTALLER_EXIT_CODE=$?
        if [ $INSTALLER_EXIT_CODE -eq 0 ]; then
          sudo rm -Rf "$TEMP_DIR"
          # the installer creates /Applications/CPLEX_Studio<ver>
          CPLEX_INSTALL_DIR=$(find /Applications -maxdepth 1 -type d -name 'CPLEX_Studio*' | head -n 1)
          sudo mv "$CPLEX_INSTALL_DIR" "$CPLEX_ROOT"
          export CPLEX_HOME="${CPLEX_ROOT}/cplex"
          export PATH="${PATH}:${CPLEX_HOME}/bin/${OSX_ARCH}"
          export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${CPLEX_HOME}/lib/${OSX_ARCH}/static_pic"
        else
          echo "CPLEX installation failed with exit code $INSTALLER_EXIT_CODE."
          exit 1
        fi
      fi
    else
      echo "CPLEX already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install Gurobi
  if [ "$install_gurobi" -eq 1 ]; then
    echo "Installing Gurobi..."
    GUROBI_ROOT="${INSTALL_ROOT}/gurobi"
    CURRENT_INSTALL_FOLDER=${GUROBI_ROOT}
    if [ ! -d "$GUROBI_ROOT" ]; then
      cd "$INSTALL_ROOT"
      GUROBI_INSTALLER="gurobi13.0.1_macos_universal2.pkg"
      curl -O "https://packages.gurobi.com/13.0/$GUROBI_INSTALLER"
      sudo installer -pkg "$GUROBI_INSTALLER" -target /
      rm "$GUROBI_INSTALLER"
      sudo mv /Library/gurobi1301 "$GUROBI_ROOT"
      export GUROBI_HOME="${GUROBI_ROOT}/macos_universal2"
      export PATH="${PATH}:${GUROBI_HOME}/bin"
      export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
      # Fix non-default installation
      if [ "$INSTALL_ROOT" != "/Library" ]; then
        GUROBI_VERSION=$(ls "${GUROBI_HOME}/lib" | grep -E '^libgurobi[0-9]+\.dylib$' | sed -E 's/^libgurobi([0-9]+)\.dylib$/\1/' | head -n1)
        install_name_tool -id "${GUROBI_HOME}/lib/libgurobi${GUROBI_VERSION}.dylib" "${GUROBI_HOME}/lib/libgurobi${GUROBI_VERSION}.dylib"
        codesign -s - -f "${GUROBI_HOME}/lib/libgurobi${GUROBI_VERSION}.dylib" "${GUROBI_HOME}/lib/libgurobi${GUROBI_VERSION}.dylib"
      fi
    else
      echo "Gurobi already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install SCIP
  if [ "$install_scip" -eq 1 ]; then
    echo "Installing SCIP..."
    SCIP_ROOT="${INSTALL_ROOT}/scip"
    CURRENT_INSTALL_FOLDER=${SCIP_ROOT}
    if [ ! -d "$SCIP_ROOT" ]; then
      brew install gcc tbb
      cd "$INSTALL_ROOT"
      SCIP_INSTALLER="scip-9.2.1"
      curl -O "https://www.scipopt.org/download/release/$SCIP_INSTALLER.tgz"
      tar -xzf "$SCIP_INSTALLER.tgz"
      rm "$SCIP_INSTALLER.tgz"
      sudo mv ./"$SCIP_INSTALLER" "$SCIP_ROOT"
      cd "$SCIP_ROOT"
      # Install PaPILO
      git clone https://github.com/scipopt/papilo.git
      cd papilo
      cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${SCIP_ROOT}/papilo"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$SCIP_ROOT"
      scip_build_flags=(
        "-DCMAKE_INSTALL_PREFIX=${SCIP_ROOT}"
        "-DAUTOBUILD=ON"
        "-DZIMPL=OFF"
        "-DPAPILO_DIR=${SCIP_ROOT}/papilo"
        "-DTBB=ON"
        "-DOPENMP=ON"
        "-Wno-dev"
      )
      cmake -S . -B build "${scip_build_flags[@]}"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$INSTALL_ROOT"
      export PATH="${PATH}:${SCIP_ROOT}/bin"
      export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${SCIP_ROOT}/lib"
    else
      echo "SCIP already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install HiGHS
  # HIPO=ON builds the HiPO interior point solver (HiGHS >= 1.14), which is
  # the only HiGHS solver able to handle convex QPs reliably; on macOS the
  # BLAS is the Accelerate framework, so no extra dependency is needed.
  if [ "$install_highs" -eq 1 ]; then
    echo "Installing HiGHS..."
    HiGHS_ROOT="${INSTALL_ROOT}/HiGHS"
    CURRENT_INSTALL_FOLDER=${HiGHS_ROOT}
    if [ ! -d "$HiGHS_ROOT" ]; then
      cd "$INSTALL_ROOT"
      git clone https://github.com/ERGO-Code/HiGHS.git
      cd HiGHS
      cmake -S . -B build -DFAST_BUILD=ON -DHIPO=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$INSTALL_ROOT"
      export PATH="${PATH}:${HiGHS_ROOT}/bin"
      export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${HiGHS_ROOT}/lib"
    else
      cd "$HiGHS_ROOT"
      LOCAL=$(git rev-parse @)
      REMOTE=$(git rev-parse @{u})
      # if the repository is not up to date
      if [ "$LOCAL" != "$REMOTE" ]; then
        git pull
        cmake -S . -B build -DFAST_BUILD=ON -DHIPO=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
        cmake --build build -j "${MAX_JOBS}"
        cmake --install build
      else
        echo "HiGHS already up to date."
      fi
      cd "$INSTALL_ROOT"
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # PIPS-IPM++ is not supported on macOS
  if [ "$install_pips" -eq 1 ]; then
    echo "Skipping PIPS-IPM++ (Linux only)."
    install_pips=0
  fi

  # Install COIN-OR CoinUtils and Osi/Clp
  if [ "$install_coinor" -eq 1 ]; then
    echo "Installing COIN-OR CoinUtils and Osi/Clp..."
    CoinOr_ROOT="${INSTALL_ROOT}/coin-or"
    CURRENT_INSTALL_FOLDER=${CoinOr_ROOT}
    if [ ! -d "$CoinOr_ROOT" ]; then
      cd "$INSTALL_ROOT"
      curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
      chmod u+x coinbrew
      # Build CoinUtils
      ./coinbrew build CoinUtils --latest-release --skip-dependencies --prefix="$CoinOr_ROOT" --tests=none
      osi_build_flags=(
        "--latest-release"
        "--skip-dependencies"
        "--prefix=$CoinOr_ROOT"
        "--tests=none"
      )
      # Build Osi with or without CPLEX
      if [ "$install_cplex" -eq 0 ]; then
        osi_build_flags+=("--without-cplex")
      else
        osi_build_flags+=(
          "--with-cplex"
          "--with-cplex-lib=-L${CPLEX_ROOT}/cplex/lib/${OSX_ARCH}/static_pic -lcplex -lm"
          "--disable-cplex-libcheck"
          "--with-cplex-incdir=${CPLEX_ROOT}/cplex/include/ilcplex"
        )
      fi
      # Build Osi with or without Gurobi
      if [ "$install_gurobi" -eq 0 ]; then
        osi_build_flags+=("--without-gurobi")
      else
        GUROBI_VERSION=$(ls "${GUROBI_ROOT}/macos_universal2/lib" | grep -E '^libgurobi[0-9]+\.dylib$' | sed -E 's/^libgurobi([0-9]+)\.dylib$/\1/' | head -n1)
        osi_build_flags+=(
          "--with-gurobi"
          "--with-gurobi-lib=-L${GUROBI_ROOT}/macos_universal2/lib -lgurobi${GUROBI_VERSION}"
          "--disable-gurobi-libcheck"
          "--with-gurobi-incdir=${GUROBI_ROOT}/macos_universal2/include"
        )
      fi
      ./coinbrew build Osi "${osi_build_flags[@]}"
      # Build Clp
      ./coinbrew build Clp --latest-release --skip-dependencies --prefix="$CoinOr_ROOT" --tests=none
      rm -Rf coinbrew build CoinUtils Osi Clp
      export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${CoinOr_ROOT}/lib"
    else
      echo "COIN-OR already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install StOpt
  if [ "$install_stopt" -eq 1 ]; then
    echo "Installing StOpt..."
    StOpt_ROOT="${INSTALL_ROOT}/StOpt"
    CURRENT_INSTALL_FOLDER=${StOpt_ROOT}
    if [ ! -d "$StOpt_ROOT" ]; then
      brew install zlib boost-mpi
      cd "$INSTALL_ROOT"
      git clone https://gitlab.com/stochastic-control/StOpt.git
      cd StOpt
      cmake -S . -B build \
            -DBUILD_PYTHON=OFF \
            -DBUILD_TEST=OFF \
            -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      cd "$INSTALL_ROOT"
    else
      cd "$StOpt_ROOT"
      LOCAL=$(git rev-parse @)
      REMOTE=$(git rev-parse @{u})
      # if the repository is not up to date
      if [ "$LOCAL" != "$REMOTE" ]; then
        git pull
        cmake -S . -B build \
              -DBUILD_PYTHON=OFF \
              -DBUILD_TEST=OFF \
              -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
        cmake --build build -j "${MAX_JOBS}"
        cmake --install build
      else
        echo "StOpt already up to date."
      fi
      cd "$INSTALL_ROOT"
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  # Install Torch
  if [ "$install_torch" -eq 1 ]; then
    echo "Installing Torch..."
    Torch_ROOT="${INSTALL_ROOT}/torch"
    CURRENT_INSTALL_FOLDER=${Torch_ROOT}
    if [ ! -d "$Torch_ROOT" ]; then
      cd "$INSTALL_ROOT"
      TORCH_VERSION="2.12.0"
      if [ "$(uname -m)" == "x86_64" ]; then # Intel arch
        # no prebuilt libtorch for macOS x86_64 past 2.2.2; Torch is optional
        echo "No prebuilt libtorch for macOS x86_64; skipping Torch."
      else # Apple Silicon arch
        TORCH_INSTALLER="libtorch-macos-arm64-${TORCH_VERSION}.zip"
        # -f fails on HTTP errors (no error page saved as a fake zip),
        # -L follows download.pytorch.org's redirect
        curl -fL -O "https://download.pytorch.org/libtorch/cpu/$TORCH_INSTALLER"
        extract_zip "$TORCH_INSTALLER" .
        rm "$TORCH_INSTALLER"
        # the archive unpacks as libtorch, rename to the version-less torch
        mv ./libtorch "$Torch_ROOT"
        export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${Torch_ROOT}/lib"
      fi
    else
      echo "Torch already installed."
    fi
    CURRENT_INSTALL_FOLDER=""
  fi

  echo "Installation completed successfully on macOS."
}

# Default values indicating if dependencies should be installed
# it works even if you use `install_*=0`
install_cplex=${install_cplex:-1}
install_gurobi=${install_gurobi:-1}
install_scip=${install_scip:-1}
install_highs=${install_highs:-1}
install_pips=${install_pips:-1}
install_stopt=${install_stopt:-1}
install_torch=${install_torch:-1}
install_lemon=${install_lemon:-1}
install_coinor=${install_coinor:-1}
install_smspp=${install_smspp:-1}

# Default value for installation root
install_root=""

# Default value for the path to a user-provided CPLEX installer
cplex_installer=""

# Default value for the maximum number of jobs
if [ -z "${MAX_JOBS:-}" ]; then
  if command -v nproc >/dev/null 2>&1; then
    MAX_JOBS=$(nproc)
  elif [ "$(uname)" = "Darwin" ]; then
    MAX_JOBS=$(sysctl -n hw.ncpu)
  else
    MAX_JOBS=1
  fi
fi

# Parse command line arguments
for arg in "$@"
do
  case $arg in
    --without-cplex)
    install_cplex=0
    shift
    ;;
    --without-gurobi)
    install_gurobi=0
    shift
    ;;
    --without-scip)
    install_scip=0
    shift
    ;;
    --without-highs)
    install_highs=0
    shift
    ;;
    --without-pips)
    install_pips=0
    shift
    ;;
    --without-stopt)
    install_stopt=0
    shift
    ;;
    --without-torch)
    install_torch=0
    shift
    ;;
    --without-lemon)
    install_lemon=0
    shift
    ;;
    --without-coinor)
    install_coinor=0
    shift
    ;;
    --without-smspp)
    install_smspp=0
    shift
    ;;
    --install-root=*)
    install_root="${arg#*=}"
    shift
    ;;
    --cplex-installer=*)
    cplex_installer="${arg#*=}"
    shift
    ;;
    *)
    ;;
  esac
done

# Remove trailing slash from install_root if present
install_root="${install_root%/}"

# Detect operating system and execute the appropriate installation function
OS="$(uname)"
case "$OS" in
"Linux")
  if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" = "ubuntu" ] || [ "$ID" = "debian" ]; then
      # Default system location where dependencies may already be installed
      DEFAULT_ROOT="/opt"
      # Check if the user has sudo access
      if sudo -n true 2>/dev/null; then
        HAS_SUDO=1
      else
        HAS_SUDO=0
      fi
      # Where to install dependencies that are not already present: an explicit
      # --install-root wins (and disables default-location probing, placing
      # everything there directly); otherwise the system default with sudo, or
      # the user's home without it (each dependency still reused from the
      # default location if found there)
      if [ -n "$install_root" ]; then
        INSTALL_ROOT="$install_root"
        DEFAULT_ROOT="$install_root"
      elif [ "$HAS_SUDO" -eq 1 ]; then
        INSTALL_ROOT="$DEFAULT_ROOT"
      else
        INSTALL_ROOT="$HOME"
      fi
      mkdir -p "$INSTALL_ROOT"
      SMSPP_ROOT="${INSTALL_ROOT}/smspp-project"
      install_on_linux
    else
      echo "This script supports Debian-based Linux distros only."
      exit 1
    fi
  else
    echo "This script supports Debian-based Linux distros only."
    exit 1
  fi
  ;;
"Darwin")
  # Check if the user has sudo access and the script is not being executed
  # on a server without display or interactive terminal
  if sudo -n true 2>/dev/null && [ -t 1 ] && [ -z "${CI:-}" ]; then
    HAS_SUDO=1
    INSTALL_ROOT="${install_root:-/Library}"
  else
    HAS_SUDO=0
    INSTALL_ROOT="${install_root:-$HOME}"
  fi
  mkdir -p "$INSTALL_ROOT"
  SMSPP_ROOT="${INSTALL_ROOT}/smspp-project"
  install_on_macos
  ;;
*)
  echo "This script does not support the detected operating system."
  exit 1
  ;;
esac

# Skip installation of SMSpp if --without-smspp is specified
if [ "$install_smspp" -eq 1 ]; then
  echo "Compiling SMSpp..."

  # Check if the SMSpp repository already exists
  if [ -d "$SMSPP_ROOT" ]; then
    cd "$SMSPP_ROOT"
    echo "SMSpp already exists. Pulling latest changes..."
    git pull --recurse-submodules
    git submodule sync --recursive
    git submodule update --init --recursive
  else
    echo "Repository not found locally. Cloning SMSpp..."
    # Check if the script is not being executed on a server without display or interactive terminal
    if [ -t 1 ] && [ -z "${CI:-}" ]; then
      git clone --branch develop https://gitlab.com/smspp/smspp-project.git "$SMSPP_ROOT"
    else
      # no way to use ccmake interactively to choose submodules, so download it all
      git clone --branch develop --recurse-submodules https://gitlab.com/smspp/smspp-project.git "$SMSPP_ROOT"
    fi
    cd "$SMSPP_ROOT"
    git submodule sync --recursive
    git submodule update --init --recursive
  fi

  # If the installation root is not the default one, update the makefile-paths
  if [[ ("$OS" == "Linux" && "$INSTALL_ROOT" != "/opt") ||
        ("$OS" == "Darwin" && "$INSTALL_ROOT" != "/Library") ]]; then

    umbrella_extlib_file="$SMSPP_ROOT/extlib/makefile-paths"
    # Create the file with the new paths of the resources for the umbrella
    {
      echo "CPLEX_ROOT = ${CPLEX_ROOT}"
      echo "SCIP_ROOT = ${SCIP_ROOT}"
      echo "GUROBI_ROOT = ${GUROBI_ROOT}"
      echo "HiGHS_ROOT = ${HiGHS_ROOT}"
      echo "StOpt_ROOT = ${StOpt_ROOT}"
      echo "CoinUtils_ROOT = ${CoinOr_ROOT}"
      echo "Osi_ROOT = ${CoinOr_ROOT}"
      echo "Clp_ROOT = ${CoinOr_ROOT}"
      echo "Torch_ROOT = ${Torch_ROOT}"
    } > "$umbrella_extlib_file"
    echo "Created $umbrella_extlib_file file."

    # If the nested submodule BundleSolver/NdoFiOracle is initialized, i.e., its
    # extlib folder exists
    if [ -d "$SMSPP_ROOT/BundleSolver/NdoFiOracle/extlib" ]; then
      ndofi_extlib_file="$SMSPP_ROOT/BundleSolver/NdoFiOracle/extlib/makefile-paths"
      # Create the file with the new paths of the resources for BundleSolver/NdoFiOracle
      {
        echo "CPLEX_ROOT = ${CPLEX_ROOT}"
        echo "GUROBI_ROOT = ${GUROBI_ROOT}"
        echo "CoinUtils_ROOT = ${CoinOr_ROOT}"
        echo "Osi_ROOT = ${CoinOr_ROOT}"
        echo "Clp_ROOT = ${CoinOr_ROOT}"
      } > "$ndofi_extlib_file"
      echo "Created $ndofi_extlib_file file."
    fi

    # If the nested submodule MCFClassSolver/MCFClass is initialized, i.e., its
    # extlib folder exists
    if [ -d "$SMSPP_ROOT/MCFClassSolver/MCFClass/extlib" ]; then
      mcf_extlib_file="$SMSPP_ROOT/MCFClassSolver/MCFClass/extlib/makefile-paths"
      # Create the file with the new paths of the resources for the MCFClassSolver/MCFClass
      {
        echo "CPLEX_ROOT = ${CPLEX_ROOT}"
      } > "$mcf_extlib_file"
      echo "Created $mcf_extlib_file file."
    fi
  fi

  # Map each skipped dependency to the SMS++ modules that hard-require it (i.e.,
  # whose CMakeLists has a `find_package(<lib> REQUIRED)`), and force those
  # modules OFF so that configuration does not fail looking for a library the
  # user asked not to install. Modules that consume a dependency optionally
  # (Torch in BundleSolverML, the CPLEX/Gurobi/SCIP/HiGHS backends in
  # MILPSolver) degrade gracefully on their own and need no mapping here.
  smspp_cmake_flags=()
  if [ "$install_stopt" -eq 0 ]; then
    # StOpt is required by SDDPBlock; InvestmentBlock pulls SDDPBlock back ON
    # through CMake's dependency resolution, so disable both.
    smspp_cmake_flags+=("-DBUILD_SDDPBlock=OFF" "-DBUILD_InvestmentBlock=OFF")
  fi
  if [ "$install_lemon" -eq 0 ]; then
    # LEMON is required by MCFLemonSolver.
    smspp_cmake_flags+=("-DBUILD_MCFLemonSolver=OFF")
  fi
  if [ "$install_coinor" -eq 0 ]; then
    # Osi/Clp (COIN-OR) are required by BundleSolver (and its NdoFiOracle).
    smspp_cmake_flags+=("-DBUILD_BundleSolver=OFF")
  fi

  # Build SMSpp
  cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${SMSPP_ROOT}" -Wno-dev "${smspp_cmake_flags[@]}"
  # Check if the script is not being executed on a server without display or interactive terminal
  if [ -t 1 ] && [ -z "${CI:-}" ]; then
    cd build
    # Ensure TERM is set to something ncurses can handle
    if [ -z "${TERM:-}" ] || [ "$TERM" = "dumb" ]; then
      export TERM=xterm
    fi
    # Run ccmake attached to the real terminal (so keyboard input works)
    ccmake .. < /dev/tty > /dev/tty 2>&1
    CCMAKE_EXIT_CODE=$?
    cd ..
    if [ $CCMAKE_EXIT_CODE -eq 0 ]; then
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
    else
      echo "ccmake fails with exit code $CCMAKE_EXIT_CODE."
      exit 1
    fi
  else
    cmake --build build -j "${MAX_JOBS}"
    cmake --install build
  fi

  # Export SMSpp lib path
  if [ "$OS" == "Linux" ]; then
    export LD_LIBRARY_PATH="${SMSPP_ROOT}/lib:$LD_LIBRARY_PATH"
    if [ "$HAS_SUDO" -eq 1 ]; then
      sh -c "echo '${SMSPP_ROOT}/lib' > /etc/ld.so.conf.d/smspp.conf"
      ldconfig
    fi
  elif [ "$OS" == "Darwin" ]; then
    export DYLD_LIBRARY_PATH="${SMSPP_ROOT}/lib:$DYLD_LIBRARY_PATH"
  fi

  # Export SMSpp paths
  SMSPP_BIN="$SMSPP_ROOT/bin"
  SMSPP_LIB="$SMSPP_ROOT/lib"
  CURRENT_SHELL=$(basename "$SHELL")
  if [ "$CURRENT_SHELL" = "zsh" ]; then
    RC_FILE="$HOME/.zshrc"
  else # default is bash
    RC_FILE="$HOME/.bashrc"
  fi

  # Eventually remove old SMSPP_BIN / SMSPP_LIB paths
  if [ -f "$RC_FILE" ]; then
    sed -i.bak "\|$SMSPP_BIN|d" "$RC_FILE"
    sed -i.bak "\|$SMSPP_LIB|d" "$RC_FILE"
  fi

  # Add the new SMSpp bin path
  echo "export PATH=\"$SMSPP_BIN:\$PATH\"" >> "$RC_FILE"
  echo ">> Added SMSpp bin path in $RC_FILE."

  # Add the new SMSpp lib path
  if [ "$OS" == "Linux" ]; then
    echo "export LD_LIBRARY_PATH=\"$SMSPP_LIB:\$LD_LIBRARY_PATH\"" >> "$RC_FILE"
    echo ">> Added SMSpp LD_LIBRARY_PATH in $RC_FILE."
    export LD_LIBRARY_PATH="$SMSPP_LIB:$LD_LIBRARY_PATH"
  elif [ "$OS" == "Darwin" ]; then
    echo "export DYLD_LIBRARY_PATH=\"$SMSPP_LIB:\$DYLD_LIBRARY_PATH\"" >> "$RC_FILE"
    echo ">> Added SMSpp DYLD_LIBRARY_PATH in $RC_FILE."
    export DYLD_LIBRARY_PATH="$SMSPP_LIB:$DYLD_LIBRARY_PATH"
  fi

  # Apply to current session as well
  export PATH="$SMSPP_BIN:$PATH"
  echo ">> Environment updated for current session."

  echo ">> To apply in current terminal run:"
  echo "   source $RC_FILE"
fi
