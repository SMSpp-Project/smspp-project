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
#     You can use the `--without-cplex` option to skip the installation of CPLEX.
#     You can use the `--without-gurobi` option to skip the installation of Gurobi.
#
# AUTHOR
#     Donato Meoli
#
# NOTES
#     Ensure that you run this script with administrative privileges.
#
# EXAMPLES
#     If you are inside the cloned repository:
#
#         ./INSTALL.sh
#
#     or:
#
#         ./INSTALL.sh --without-cplex
#     if you do not have a CPLEX license.
#
#         ./INSTALL.sh --without-gurobi
#     if you do not have a Gurobi license.
#
#     If you have not yet cloned the SMS++ repository, you can run the script directly:
#
#     Using `curl`:
#         If you want to install SMS++ with all dependencies:
#             curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash
#
#         If you do not have a license for CPLEX and/or Gurobi, or if you just want to install SMS++ without them:
#             curl -s https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo --without-cplex --without-gurobi bash
#
#     Using `wget`:
#         If you want to install SMS++ with all dependencies:
#             wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo bash
#
#         If you do not have a license for CPLEX and/or Gurobi, or if you just want to install SMS++ without them:
#             wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | sudo --without-cplex --without-gurobi bash
# ------------------------------------------------------------------------------

# Function to install dependencies on Ubuntu
install_on_ubuntu() {
  set -e  # Exit immediately if a command exits with a non-zero status

  echo "Starting the installation process on Ubuntu..."

  # Update packages and install basic requirements
  echo "Updating system and installing basic requirements..."
  apt-get update -q
  apt-get install -y -q build-essential clang cmake cmake-curses-gui git curl xterm

  # Install Boost libraries
  echo "Installing Boost libraries..."
  apt-get install -y -q libboost-dev libboost-system-dev libboost-timer-dev libboost-mpi-dev libboost-random-dev

  # Install OpenMP
  echo "Installing OpenMP..."
  apt-get install -y -q libomp-dev

  # Install Eigen
  echo "Installing Eigen..."
  apt-get install -y -q libeigen3-dev

  # Install NetCDF-C++
  echo "Installing NetCDF-C++..."
  apt-get install -y -q libnetcdf-c++4-dev

  # Install CPLEX
  if [ "$install_cplex" -eq 1 ]; then
    echo "Installing CPLEX..."
    CPLEX_ROOT="/opt/ibm/ILOG/CPLEX_Studio"
    if [ ! -d "$CPLEX_ROOT" ]; then
      cd /opt
      CPLEX_INSTALLER="cplex_studio2211.linux_x86_64.bin"
      # the CPLEX_URL is always given by the same prefix, i.e.:
      # "https://drive.usercontent.google.com/download?id=" +
      # the id code suffix in the Drive sharing link, i.e.:
      # https://drive.google.com/file/d/ 12JpuzOAjnuQK6tq2LLolIgmlmKTmOP4x /view?usp=sharing
      CPLEX_URL="https://drive.usercontent.google.com/download?id=12JpuzOAjnuQK6tq2LLolIgmlmKTmOP4x"
      uuid=$(curl -sL "$CPLEX_URL" | grep -oP 'name="uuid" value="\K[^"]+')
      if [ -n "$uuid" ]; then
          curl -o "$CPLEX_INSTALLER" "$CPLEX_URL&export=download&authuser=0&confirm=t&uuid=$uuid"
          chmod u+x "$CPLEX_INSTALLER"
          cat <<EOL > installer.properties
INSTALLER_UI=silent
LICENSE_ACCEPTED=TRUE
USER_INSTALL_DIR=$CPLEX_ROOT
EOL
          # run the CPLEX installer in a xterm subshell
          # (calling gnome-terminal as a subshell does not work with sudo)
          xterm -e ./"$CPLEX_INSTALLER" -f ./installer.properties &
          wait $! # wait for CPLEX installer to finish
          INSTALLER_EXIT_CODE=$?
          if [ $INSTALLER_EXIT_CODE -eq 0 ]; then
            rm "$CPLEX_INSTALLER" installer.properties
            #mv ./ibm/ILOG/CPLEX_Studio2211 "$CPLEX_ROOT"
            export CPLEX_HOME="${CPLEX_ROOT}/cplex"
            export PATH="${PATH}:${CPLEX_HOME}/bin/x86-64_linux"
            export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CPLEX_HOME}/lib/x86-64_linux"
            sh -c "echo '${CPLEX_HOME}/lib' > /etc/ld.so.conf.d/cplex.conf"
            ldconfig
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
  fi

  # Install Gurobi
  if [ "$install_gurobi" -eq 1 ]; then
      echo "Installing Gurobi..."
      GUROBI_ROOT="/opt/gurobi"
      if [ ! -d "$GUROBI_ROOT" ]; then
          cd /opt
          GUROBI_INSTALLER="gurobi10.0.3_linux64.tar.gz"
          curl -O "https://packages.gurobi.com/10.0/$GUROBI_INSTALLER"
          tar -xvf "$GUROBI_INSTALLER"
          rm "$GUROBI_INSTALLER"
          mv ./gurobi1003 "$GUROBI_ROOT"
          export GUROBI_HOME="${GUROBI_ROOT}/linux64"
          export PATH="${PATH}:${GUROBI_HOME}/bin"
          export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
          sh -c "echo '${GUROBI_HOME}/lib' > /etc/ld.so.conf.d/gurobi.conf"
          ldconfig
      else
          echo "Gurobi already installed."
      fi
  fi

  # Install SCIP
  echo "Installing SCIP..."
  SCIP_ROOT="/opt/scip"
  if [ ! -d "$SCIP_ROOT" ]; then
      apt-get install -y -q gfortran libtbb-dev
      cd /opt
      SCIP_INSTALLER="SCIPOptSuite-9.0.0-Linux-ubuntu22.sh"
      curl -O "https://www.scipopt.org/download/release/$SCIP_INSTALLER"
      chmod u+x "$SCIP_INSTALLER"
      ./"$SCIP_INSTALLER" --prefix="$SCIP_ROOT" --exclude-subdir --skip-license
      rm "$SCIP_INSTALLER"
      sh -c "echo '${SCIP_ROOT}/lib' > /etc/ld.so.conf.d/scip.conf"
      ldconfig
  else
      echo "SCIP already installed."
  fi

  # Install HiGHS
  echo "Installing HiGHS..."
  HiGHS_ROOT=/opt/HiGHS
  if [ ! -d "$HiGHS_ROOT" ]; then
    cd /opt
    git clone https://github.com/ERGO-Code/HiGHS.git
    cd HiGHS
    mkdir build
    cd build
    cmake -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT" ..
    cmake --build .
    cmake --install .
    sh -c "echo '${HiGHS_ROOT}/lib' > /etc/ld.so.conf.d/highs.conf"
    ldconfig
  else
    cd "$HiGHS_ROOT"
    git remote update
    LOCAL=$(git rev-parse @)
    REMOTE=$(git rev-parse @{u})
    # if the repository is not up to date
    if [ "$LOCAL" != "$REMOTE" ]; then
      git pull
      cd build
      cmake -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT" ..
      cmake --build .
      cmake --install .
    else
      echo "HiGHS already up to date."
    fi
    cd /opt
  fi

  # Install COIN-OR CoinUtils and Osi/Clp
  echo "Installing COIN-OR CoinUtils and Osi/Clp..."
  apt-get install -y -q coinor-libcoinutils-dev libbz2-dev liblapack-dev libopenblas-dev
  CoinOr_ROOT=/opt/coin-or
  if [ ! -d "$CoinOr_ROOT" ]; then
    cd /opt
    curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
    chmod u+x coinbrew
    # Build CoinUtils
    ./coinbrew build CoinUtils --latest-release --skip-dependencies --prefix="$CoinOr_ROOT" --tests=none
    # Build Osi with or without CPLEX
    osi_build_flags=(
      "--latest-release"
      "--skip-dependencies"
      "--prefix=$CoinOr_ROOT"
      "--tests=none"
    )
    if [ "$install_cplex" -eq 0 ]; then
      osi_build_flags+=("--without-cplex")
    else
      osi_build_flags+=(
        "--with-cplex"
        "--with-cplex-lib=-L${CPLEX_ROOT}/cplex/lib/x86-64_linux/static_pic -lcplex -lilocplex -lm -ldl -lpthread"
        "--with-cplex-incdir=${CPLEX_ROOT}/cplex/include/ilcplex"
      )
    fi
    # Build Osi with or without Gurobi
    if [ "$install_gurobi" -eq 0 ]; then
      osi_build_flags+=("--without-gurobi")
    else
      osi_build_flags+=(
        "--with-gurobi"
        "--with-gurobi-lib=-L${GUROBI_ROOT}/linux64/lib -lgurobi100"
        "--with-gurobi-incdir=${GUROBI_ROOT}/linux64/include"
      )
    fi
    ./coinbrew build Osi "${osi_build_flags[@]}"
    # Build Clp
    ./coinbrew build Clp --latest-release --skip-dependencies --prefix="$CoinOr_ROOT" --tests=none
    rm -R coinbrew build CoinUtils Osi Clp
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$CoinOr_ROOT/lib"
    sh -c "echo '$CoinOr_ROOT/lib' > /etc/ld.so.conf.d/coin-or.conf"
    ldconfig
  else
    echo "COIN-OR already installed."
  fi

  # Install StOpt
  echo "Installing StOpt..."
  StOpt_ROOT=/opt/StOpt
  apt-get install -y -q zlib1g-dev
  if [ ! -d "$StOpt_ROOT" ]; then
    cd /opt
    git clone https://gitlab.com/stochastic-control/StOpt
    cd StOpt
    mv ./doc /opt # TODO remove when the doc bug in StOpt will be fixed
    mkdir build
    cd build
    cmake -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT" ..
    cmake --build .
    cmake --install .
    mv /opt/doc StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
  else
    cd "$StOpt_ROOT"
    LOCAL=$(git rev-parse @)
    REMOTE=$(git rev-parse @{u})
    # if the repository is not up to date
    if [ "$LOCAL" != "$REMOTE" ]; then
      git pull
      mv ./doc /opt # TODO remove when the doc bug in StOpt will be fixed
      cd build
      cmake -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT" ..
      cmake --build .
      cmake --install .
      mv /opt/doc StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
    else
      echo "StOpt already up to date."
    fi
    cd /opt
  fi

  echo "Installation completed successfully on Ubuntu."
}

# Function to install dependencies on macOS
install_on_macos() {
  set -e  # Exit immediately if a command exits with a non-zero status

  echo "Starting the installation process on macOS..."

  # Install Homebrew
  echo "Installing Homebrew..."
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

  # Install Xcode Command Line Tools (includes build-essential and clang)
  echo "Installing Xcode Command Line Tools..."
  xcode-select --install

  # Install basic requirements
  echo "Installing basic requirements..."
  brew install cmake git

  # Install Boost libraries
  echo "Installing Boost libraries..."
  brew install boost

  # Install OpenMP
  echo "Installing OpenMP..."
  brew install libomp

  # Install Eigen
  echo "Installing Eigen..."
  brew install eigen

  # Install NetCDF
  echo "Installing NetCDF..."
  brew install netcdf

  # Install CPLEX
  if [ "$install_cplex" -eq 1 ]; then
    echo "Installing CPLEX..."
    CPLEX_ROOT="/Applications/CPLEX_Studio"
    if [ ! -d "$CPLEX_ROOT" ]; then
      cd /Applications
      CPLEX_INSTALLER="cplex_studio2211.osx.zip"
      # the CPLEX_URL is always given by the same prefix, i.e.:
      # "https://drive.usercontent.google.com/download?id=" +
      # the id code suffix in the Drive sharing link, i.e.:
      # https://drive.google.com/file/d/ 1_xE4MBohevx3Bb_lpl8euXyYWKS_zcVK /view?usp=sharing
      CPLEX_URL="https://drive.usercontent.google.com/download?id=1_xE4MBohevx3Bb_lpl8euXyYWKS_zcVK"
      uuid=$(curl -sL "$CPLEX_URL" | grep -oP 'name="uuid" value="\K[^"]+')
      if [ -n "$uuid" ]; then
        curl -o "$CPLEX_INSTALLER" "$CPLEX_URL&export=download&authuser=0&confirm=t&uuid=$uuid"
        tar -xvf "$CPLEX_INSTALLER"
        # TODO
        rm "$CPLEX_INSTALLER"
        mv ./CPLEX_Studio2211 "$CPLEX_ROOT"
        export CPLEX_HOME="${CPLEX_ROOT}"
        export PATH="${PATH}:${CPLEX_HOME}/bin/x86-64_osx"
        export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${CPLEX_HOME}/lib/x86-64_osx"
      else
        echo "Error: unable to find the UUID value in the response. The CPLEX download link could not be constructed."
        exit 1
      fi
    else
      echo "CPLEX already installed."
    fi
  fi

  # Install Gurobi
  if [ "$install_gurobi" -eq 1 ]; then
    echo "Installing Gurobi..."
    GUROBI_ROOT="/Library/gurobi"
    if [ ! -d "$GUROBI_ROOT" ]; then
      cd /Library
      GUROBI_INSTALLER="gurobi10.0.3_macos_universal2.pkg"
      curl -O "https://packages.gurobi.com/10.0/$GUROBI_INSTALLER"
      installer -pkg "$GUROBI_INSTALLER" -target /
      rm "$GUROBI_INSTALLER"
      mv ./gurobi1003 "$GUROBI_ROOT"
      export GUROBI_HOME="$GUROBI_ROOT"
      export PATH="${PATH}:${GUROBI_HOME}/bin"
      export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
    else
      echo "Gurobi already installed."
    fi
  fi

  # Install SCIP
  echo "Installing SCIP..."
  SCIP_ROOT="/Library/scip"
  if [ ! -d "$SCIP_ROOT" ]; then
    brew install gcc tbb
    cd /Library
    SCIP_INSTALLER="SCIPOptSuite-9.0.0-Darwin.sh"
    curl -O "https://www.scipopt.org/download/release/$SCIP_INSTALLER"
    chmod u+x "$SCIP_INSTALLER"
    ./"$SCIP_INSTALLER" --prefix="$SCIP_ROOT" --exclude-subdir --skip-license
    rm "$SCIP_INSTALLER"
  else
    echo "SCIP already installed."
  fi

  # Install HiGHS
  echo "Installing HiGHS..."
  HiGHS_ROOT="/Library/HiGHS"
  if [ ! -d "$HiGHS_ROOT" ]; then
    cd /Library
    git clone https://github.com/ERGO-Code/HiGHS.git
    cd HiGHS
    mkdir build
    cd build
    cmake -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT" ..
    cmake --build .
    cmake --install .
  else
    cd "$HiGHS_ROOT"
    LOCAL=$(git rev-parse @)
    REMOTE=$(git rev-parse @{u})
    # if the repository is not up to date
    if [ "$LOCAL" != "$REMOTE" ]; then
      git pull
      cd build
      cmake -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX="$HiGHS_ROOT" ..
      cmake --build .
      cmake --install .
    else
      echo "HiGHS already up to date."
    fi
    cd /Library
  fi

  # Install COIN-OR CoinUtils and Osi/Clp
  echo "Installing COIN-OR CoinUtils and Osi/Clp..."
  CoinOr_ROOT="/Library/coin-or"
  if [ ! -d "$CoinOr_ROOT" ]; then
    brew install coinutils bz2 lapack openblas
    cd /Library
    curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
    chmod u+x coinbrew
    # Build CoinUtils
    ./coinbrew fetch CoinUtils --no-prompt
    # Build Osi with or without CPLEX
    osi_build_flags=(
      "--prefix=$CoinOr_ROOT"
      "--no-prompt"
      "--tests=none"
    )
    if [ "$install_cplex" -eq 0 ]; then
      osi_build_flags+=("--without-cplex")
    else
      osi_build_flags+=(
        "--with-cplex"
        "--with-cplex-lib=-L${CPLEX_HOME}/lib/x86-64_osx/static_pic -lcplex -lilocplex -lm -ldl -lpthread"
        "--with-cplex-incdir=${CPLEX_HOME}/include/ilcplex"
      )
    fi
    # Build Osi with or without Gurobi
    if [ "$install_gurobi" -eq 0 ]; then
      osi_build_flags+=("--without-gurobi")
    else
      osi_build_flags+=(
        "--with-gurobi"
        "--with-gurobi-lib=-L${GUROBI_HOME}/lib -lgurobi100"
        "--with-gurobi-incdir=${GUROBI_HOME}/include"
      )
    fi
    ./coinbrew build Osi "${osi_build_flags[@]}"
    # Build Clp
    ./coinbrew build Clp --prefix="$CoinOr_ROOT" --tests=none
    rm -R coinbrew build CoinUtils Osi Clp
    export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:$CoinOr_ROOT/lib"
  else
    echo "COIN-OR already installed."
  fi

  # Install StOpt
  echo "Installing StOpt..."
  StOpt_ROOT="/Library/StOpt"
  if [ ! -d "$StOpt_ROOT" ]; then
    brew install zlib
    cd /Library
    git clone https://gitlab.com/stochastic-control/StOpt
    cd StOpt
    mv ./doc /opt # TODO remove when the doc bug in StOpt will be fixed
    mkdir build
    cd build
    cmake -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT" ..
    cmake --build .
    cmake --install .
    mv /opt/doc StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
  else
    cd "$StOpt_ROOT"
    LOCAL=$(git rev-parse @)
    REMOTE=$(git rev-parse @{u})
    # if the repository is not up to date
    if [ "$LOCAL" != "$REMOTE" ]; then
      git pull
      mv ./doc /opt # TODO remove when the doc bug in StOpt will be fixed
      cd build
      cmake -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF -DCMAKE_INSTALL_PREFIX="$StOpt_ROOT" ..
      cmake --build .
      cmake --install .
      mv /opt/doc StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
    else
      echo "StOpt already up to date."
    fi
    cd /Library
  fi

  echo "Installation completed successfully on macOS."
}

# Default values indicating if CPLEX and Gurobi should be installed
# it works even if you use `install_cplex=0` or `install_gurobi=0`
install_cplex=${install_cplex:-1}
install_gurobi=${install_gurobi:-1}

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
    *)
    ;;
  esac
done

# Detect operating system and execute the appropriate installation function
OS="$(uname)"
case "$OS" in
"Linux")
  if [ -f /etc/lsb-release ]; then
    . /etc/lsb-release
    if [ "$DISTRIB_ID" = "Ubuntu" ]; then
      install_on_ubuntu
      CMAKE_PREFIX="/opt/SMSpp"
    else
      echo "This script supports Ubuntu only."
      exit 1
    fi
  else
    echo "This script supports Ubuntu only."
    exit 1
  fi
  ;;
"Darwin")
  install_on_macos
  CMAKE_PREFIX="/Library/SMSpp"
  ;;
*)
  echo "This script does not support the detected operating system."
  exit 1
  ;;
esac

# Install SMSpp
echo "Compiling SMSpp..."

# Check if we are inside the SMSpp repository by looking for a .git directory
if [ -d ".git" ]; then
  echo "Inside SMSpp repository. Pulling latest changes..."
  git pull
else
  SMSPP_ROOT="smspp-project"
  if [ ! -d "$SMSPP_ROOT" ]; then
    echo "Repository not found locally. Cloning SMSpp..."
    git clone -b develop https://gitlab.com/smspp/smspp-project.git "$SMSPP_ROOT"
    cd "$SMSPP_ROOT"
  else
    cd "$SMSPP_ROOT"
    git pull
  fi
fi

# Build Debug
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake -DCMAKE_INSTALL_PREFIX="$CMAKE_PREFIX" -DCMAKE_BUILD_TYPE=Debug -Wno-dev ..
# run ccmake in a subshell
sh -c "ccmake .." & # select submodules, then Configure and Generate the build files
wait # wait for ccmake to finish
cmake --build . --config Debug
cmake --install . --config Debug
cd ..

# Build Release
mkdir -p cmake-build-release
cd cmake-build-release
cmake -DCMAKE_INSTALL_PREFIX="$CMAKE_PREFIX" -DCMAKE_BUILD_TYPE=Release -Wno-dev ..
# run ccmake in a subshell
sh -c "ccmake .." & # select submodules, then Configure and Generate the build files
wait # wait for ccmake to finish
cmake --build . --config Release
cmake --install . --config Release
cd ..
