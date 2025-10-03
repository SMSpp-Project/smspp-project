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
#     You can use the `--without-cplex` option to skip the installation of CPLEX.
#     You can use the `--without-gurobi` option to skip the installation of Gurobi.
#     You can use the `--without-scip` option to skip the installation of SCIP.
#     You can use the `--without-highs` option to skip the installation of HiGHS.
#     You can use the `--without-stopt` option to skip the installation of StOpt.
#     You can use the `--without-coinor` option to skip the installation of COIN-OR.
#     You can use the `--without-smspp` option to skip the installation of SMS++.
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
    apt-get install -y -q libboost-dev libboost-timer-dev libboost-random-dev libboost-mpi-dev

    # Install OpenMP
    echo "Installing OpenMP..."
    apt-get install -y -q libomp-dev

    # Install Eigen
    echo "Installing Eigen..."
    apt-get install -y -q libeigen3-dev

    # Install NetCDF-C++
    echo "Installing NetCDF-C++..."
    apt-get install -y -q libnetcdf-c++4-dev
  fi

  # Install CPLEX
  if [ "$install_cplex" -eq 1 ]; then
    echo "Installing CPLEX..."
    CPLEX_ROOT="${INSTALL_ROOT}/ibm/ILOG/CPLEX_Studio"
    CURRENT_INSTALL_FOLDER="${INSTALL_ROOT}/ibm"
    if [ ! -d "$CPLEX_ROOT" ]; then
      cd "$INSTALL_ROOT"
      CPLEX_INSTALLER="cplex_studio2211.linux_x86_64.bin"
      # the CPLEX_URL is always given by the same prefix, i.e.:
      # "https://drive.usercontent.google.com/download?id=" +
      # the id code suffix in the Drive sharing link, i.e.:
      # https://drive.google.com/file/d/ 12JpuzOAjnuQK6tq2LLolIgmlmKTmOP4x /view?usp=sharing
      CPLEX_URL="https://drive.usercontent.google.com/download?id=12JpuzOAjnuQK6tq2LLolIgmlmKTmOP4x"
      uuid=$(curl -sL "$CPLEX_URL" | grep -oE 'name="uuid" value="[^"]+"' | cut -d '"' -f 4)
      if [ -n "$uuid" ]; then
        curl -o "$CPLEX_INSTALLER" "$CPLEX_URL&export=download&authuser=0&confirm=t&uuid=$uuid"
        chmod u+x "$CPLEX_INSTALLER"
        cat <<EOL > installer.properties
INSTALLER_UI=silent
LICENSE_ACCEPTED=TRUE
USER_INSTALL_DIR=$CPLEX_ROOT
EOL
        ./"$CPLEX_INSTALLER" -f ./installer.properties &
        wait $! # wait for CPLEX installer to finish
        INSTALLER_EXIT_CODE=$?
        if [ $INSTALLER_EXIT_CODE -eq 0 ]; then
          rm "$CPLEX_INSTALLER" installer.properties
          #mv ./ibm/ILOG/CPLEX_Studio2211 "$CPLEX_ROOT"
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
      else
        echo "Error: unable to find the UUID value in the response. The CPLEX download link could not be constructed."
        exit 1
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
      GUROBI_INSTALLER="gurobi12.0.1_linux64.tar.gz"
      curl -O "https://packages.gurobi.com/12.0/$GUROBI_INSTALLER"
      tar -xzf "$GUROBI_INSTALLER"
      rm "$GUROBI_INSTALLER"
      mv ./gurobi1201 "$GUROBI_ROOT"
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
    SCIP_ROOT="${INSTALL_ROOT}/scip"
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
  if [ "$install_highs" -eq 1 ]; then
    echo "Installing HiGHS..."
    HiGHS_ROOT="${INSTALL_ROOT}/HiGHS"
    CURRENT_INSTALL_FOLDER=${HiGHS_ROOT}
    if [ ! -d "$HiGHS_ROOT" ]; then
      cd "$INSTALL_ROOT"
      git clone https://github.com/ERGO-Code/HiGHS.git
      cd HiGHS
      cmake -S . -B build -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
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
          cmake -S . -B build -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
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

  # Install COIN-OR CoinUtils and Osi/Clp
  if [ "$install_coinor" -eq 1 ]; then
    echo "Installing COIN-OR CoinUtils and Osi/Clp..."
    CoinOr_ROOT="${INSTALL_ROOT}/coin-or"
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
    StOpt_ROOT="${INSTALL_ROOT}/StOpt"
    CURRENT_INSTALL_FOLDER=${StOpt_ROOT}
    if [ "$HAS_SUDO" -eq 1 ]; then
      apt-get install -y -q zlib1g-dev
    fi
    if [ ! -d "$StOpt_ROOT" ]; then
      cd "$INSTALL_ROOT"
      git clone https://gitlab.com/stochastic-control/StOpt.git
      cd StOpt
      mv ./doc "${INSTALL_ROOT}" # TODO remove when the doc bug in StOpt will be fixed
      cmake -S . -B build \
            -DBUILD_PYTHON=OFF \
            -DBUILD_TEST=OFF \
            -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      mv "${INSTALL_ROOT}/doc" "$StOpt_ROOT" # TODO remove when the doc bug in StOpt will be fixed
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
          mv ./doc "${INSTALL_ROOT}"  # TODO remove when the doc bug in StOpt will be fixed
          cmake -S . -B build \
                -DBUILD_PYTHON=OFF \
                -DBUILD_TEST=OFF \
                -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
          cmake --build build -j "${MAX_JOBS}"
          cmake --install build
          mv "${INSTALL_ROOT}/doc" "$StOpt_ROOT" # TODO remove when the doc bug in StOpt will be fixed
        else
          echo "StOpt already up to date."
        fi
        cd "$INSTALL_ROOT"
      fi
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
  brew install open-mpi

  # Install Boost libraries
  echo "Installing Boost libraries..."
  brew install boost

  # Install Eigen
  echo "Installing Eigen..."
  brew install eigen

  # Install NetCDF
  echo "Installing NetCDF..."
  brew install hdf5 netcdf netcdf-cxx

  # Install CPLEX
  if [ "$install_cplex" -eq 1 ]; then
    echo "Installing CPLEX..."
    CPLEX_ROOT="${INSTALL_ROOT}/CPLEX_Studio"
    CURRENT_INSTALL_FOLDER=${CPLEX_ROOT}
    if [ ! -d "$CPLEX_ROOT" ]; then
      cd "$INSTALL_ROOT"
      if [ "$OSX_ARCH" == "x86-64_osx" ]; then # Intel arch
        CPLEX_INSTALLER="cplex_studio2211.osx.zip"
        # the CPLEX_URL is always given by the same prefix, i.e.:
        # "https://drive.usercontent.google.com/download?id=" +
        # the id code suffix in the Drive sharing link, i.e.:
        # https://drive.google.com/file/d/ 1_xE4MBohevx3Bb_lpl8euXyYWKS_zcVK /view?usp=sharing
        CPLEX_URL="https://drive.usercontent.google.com/download?id=1_xE4MBohevx3Bb_lpl8euXyYWKS_zcVK"
        CPLEX_NAME="cplex_studio2211-osx"
      else # Apple Silicon MX arch
        CPLEX_INSTALLER="cplex_studio2211.osx.arm64.zip"
        # the CPLEX_URL is always given by the same prefix, i.e.:
        # "https://drive.usercontent.google.com/download?id=" +
        # the id code suffix in the Drive sharing link, i.e.:
        # https://drive.google.com/file/d/ 1HAEILAjuHXnghVgjQ66jP9sfub-vDq3r /view?usp=sharing
        CPLEX_URL="https://drive.usercontent.google.com/download?id=1HAEILAjuHXnghVgjQ66jP9sfub-vDq3r"
        CPLEX_NAME="cplex_studio2211-osx-arm64"
      fi
      uuid=$(curl -sL "$CPLEX_URL" | grep -oE 'name="uuid" value="[^"]+"' | cut -d '"' -f 4)
      if [ -n "$uuid" ]; then
        curl -o "$CPLEX_INSTALLER" "$CPLEX_URL&export=download&authuser=0&confirm=t&uuid=$uuid"
        # Create a temporary directory to extract the files
        TEMP_DIR="/tmp/cplex_install"
        mkdir -p "$TEMP_DIR"
        # Extract directly into the temporary directory
        sudo unzip "$CPLEX_INSTALLER" -d "$TEMP_DIR"
        # Remove macOS quarantine attribute to allow execution of unsigned app
        sudo xattr -r -d com.apple.quarantine "${TEMP_DIR}/${CPLEX_NAME}.app"
        # Launch the installer
        sudo "${TEMP_DIR}/${CPLEX_NAME}.app/Contents/MacOS/${CPLEX_NAME}" &
        wait $! # wait for the installer to finish
        INSTALLER_EXIT_CODE=$?
        if [ $INSTALLER_EXIT_CODE -eq 0 ]; then
          sudo rm -Rf "$CPLEX_INSTALLER" "$TEMP_DIR"
          sudo mv "/Applications/CPLEX_Studio2211" "$CPLEX_ROOT"
          export CPLEX_HOME="${CPLEX_ROOT}/cplex"
          export PATH="${PATH}:${CPLEX_HOME}/bin/${OSX_ARCH}"
          export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${CPLEX_HOME}/lib/${OSX_ARCH}/static_pic"
        else
          echo "CPLEX installation failed with exit code $INSTALLER_EXIT_CODE."
          exit 1
        fi
      else
        echo "Error: unable to find the UUID value in the response. The CPLEX download link could not be constructed."
        exit 1
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
      GUROBI_INSTALLER="gurobi12.0.1_macos_universal2.pkg"
      curl -O "https://packages.gurobi.com/12.0/$GUROBI_INSTALLER"
      sudo installer -pkg "$GUROBI_INSTALLER" -target /
      rm "$GUROBI_INSTALLER"
      sudo mv /Library/gurobi1201 "$GUROBI_ROOT"
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
  if [ "$install_highs" -eq 1 ]; then
    echo "Installing HiGHS..."
    HiGHS_ROOT="${INSTALL_ROOT}/HiGHS"
    CURRENT_INSTALL_FOLDER=${HiGHS_ROOT}
    if [ ! -d "$HiGHS_ROOT" ]; then
      cd "$INSTALL_ROOT"
      git clone https://github.com/ERGO-Code/HiGHS.git
      cd HiGHS
      cmake -S . -B build -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
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
        cmake -S . -B build -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT"
        cmake --build build -j "${MAX_JOBS}"
        cmake --install build
      else
        echo "HiGHS already up to date."
      fi
      cd "$INSTALL_ROOT"
    fi
    CURRENT_INSTALL_FOLDER=""
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
      sudo mv ./doc "${INSTALL_ROOT}" # TODO remove when the doc bug in StOpt will be fixed
      cmake -S . -B build \
            -DBUILD_PYTHON=OFF \
            -DBUILD_TEST=OFF \
            -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
      cmake --build build -j "${MAX_JOBS}"
      cmake --install build
      sudo mv "${INSTALL_ROOT}/doc" "$StOpt_ROOT" # TODO remove when the doc bug in StOpt will be fixed
      cd "$INSTALL_ROOT"
    else
      cd "$StOpt_ROOT"
      LOCAL=$(git rev-parse @)
      REMOTE=$(git rev-parse @{u})
      # if the repository is not up to date
      if [ "$LOCAL" != "$REMOTE" ]; then
        git pull
        sudo mv ./doc "${INSTALL_ROOT}" # TODO remove when the doc bug in StOpt will be fixed
        cmake -S . -B build \
              -DBUILD_PYTHON=OFF \
              -DBUILD_TEST=OFF \
              -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT"
        cmake --build build -j "${MAX_JOBS}"
        cmake --install build
        sudo mv "${INSTALL_ROOT}/doc" "$StOpt_ROOT" # TODO remove when the doc bug in StOpt will be fixed
      else
        echo "StOpt already up to date."
      fi
      cd "$INSTALL_ROOT"
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
install_stopt=${install_stopt:-1}
install_coinor=${install_coinor:-1}
install_smspp=${install_smspp:-1}

# Default value for installation root
install_root=""

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
    --without-stopt)
    install_stopt=0
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
      INSTALL_ROOT="${install_root:-/opt}"
      mkdir -p "$INSTALL_ROOT"
      # Check if the user has sudo access and the script is not being executed
      # on a server without display or interactive terminal
      if sudo -n true 2>/dev/null && [ -t 1 ] && [ -z "${CI:-}" ]; then
        HAS_SUDO=1
        SMSPP_ROOT="${INSTALL_ROOT}/smspp-project"
      else
        HAS_SUDO=0
        SMSPP_ROOT="${HOME}/smspp-project"
      fi
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
    } > "$umbrella_extlib_file"
    echo "Created $umbrella_extlib_file file."

    # If the submodule BundleSolver is initialized, i.e., the folder is not empty
    if [ -d "$SMSPP_ROOT/BundleSolver" ] && [ -n "$(ls -A "$SMSPP_ROOT/BundleSolver")" ]; then
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

    # If the submodule MCFBlock is initialized, i.e., the folder is not empty
    if [ -d "$SMSPP_ROOT/MCFBlock" ] && [ -n "$(ls -A "$SMSPP_ROOT/MCFBlock")" ]; then
      mcf_extlib_file="$SMSPP_ROOT/MCFBlock/MCFClass/extlib/makefile-paths"
      # Create the file with the new paths of the resources for the MCFBlock/MCFClass
      {
        echo "CPLEX_ROOT = ${CPLEX_ROOT}"
      } > "$mcf_extlib_file"
      echo "Created $mcf_extlib_file file."
    fi
  fi

  # Build SMSpp
  cmake -S . -B build -DCMAKE_INSTALL_PREFIX="${SMSPP_ROOT}" -Wno-dev
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

  # Export SMSpp bin path
  SMSPP_BIN="$SMSPP_ROOT/bin"
  CURRENT_SHELL=$(basename "$SHELL")
  if [ "$CURRENT_SHELL" = "zsh" ]; then
    RC_FILE="$HOME/.zshrc"
  else # default is bash
    RC_FILE="$HOME/.bashrc"
  fi
  # Eventually remove old SMSPP_BIN path
  if [ -f "$RC_FILE" ]; then
    sed -i.bak "\|$SMSPP_BIN|d" "$RC_FILE"
  fi
  # Add the new SMSPP_BIN path
  echo "export PATH=\"$SMSPP_BIN:\$PATH\"" >> "$RC_FILE"
  echo ">> Added SMSpp bin path in $RC_FILE."
  . "$RC_FILE"
fi
