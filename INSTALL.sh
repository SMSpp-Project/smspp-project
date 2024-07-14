#!/bin/bash

# Function to install dependencies on Ubuntu
install_on_ubuntu() {
  set -e # Exit immediately if a command exits with a non-zero status

  echo "Starting the installation process on Ubuntu..."

  # Update packages and install basic requirements
  echo "Updating system and installing basic requirements..."
  apt-get update
  apt-get install -y build-essential clang cmake cmake-curses-gui git curl

  # Install Boost libraries
  echo "Installing Boost libraries..."
  apt-get install -y libboost-dev libboost-system-dev libboost-timer-dev libboost-mpi-dev libboost-random-dev

  # Install OpenMP
  apt-get install -y libomp-dev

  # Install Eigen
  echo "Installing Eigen..."
  apt-get install -y libeigen3-dev

  # Install NetCDF-C++
  echo "Installing NetCDF-C++..."
  apt-get install -y libnetcdf-c++4-dev

  # Install CPLEX
  if [ $install_cplex -eq 1 ]; then
    echo "Installing CPLEX..."
    cd /opt
    CPLEX_INSTALLER="cplex_studio2211.linux_x86_64.bin"
    curl -O https://drive.google.com/uc?id=12JpuzOAjnuQK6tq2LLolIgmlmKTmOP4x
    chmod u+x $CPLEX_INSTALLER
    ./$CPLEX_INSTALLER
    rm $CPLEX_INSTALLER
    mv ./ibm/ILOG/CPLEX_Studio2211 /opt/ibm/ILOG/CPLEX_Studio
    export CPLEX_HOME="/opt/ibm/ILOG/CPLEX_Studio/cplex"
    export PATH="${PATH}:${CPLEX_HOME}/bin/x86-64_linux"
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${CPLEX_HOME}/lib/x86-64_linux"
    sh -c "echo '${CPLEX_HOME}/lib' > /etc/ld.so.conf.d/cplex.conf"
    ldconfig
  fi

  # Install Gurobi
  echo "Installing Gurobi..."
  cd /opt
  GUROBI_INSTALLER="gurobi10.0.3_linux64.tar.gz"
  curl -O https://packages.gurobi.com/10.0/$GUROBI_INSTALLER
  tar -xvf $GUROBI_INSTALLER
  rm $GUROBI_INSTALLER
  mv ./gurobi1003 /opt/gurobi
  export GUROBI_HOME="/opt/gurobi/linux64"
  export PATH="${PATH}:${GUROBI_HOME}/bin"
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
  sh -c "echo '${GUROBI_HOME}/lib' > /etc/ld.so.conf.d/gurobi.conf"
  ldconfig

  # Install SCIP
  echo "Installing SCIP..."
  apt-get install -y gfortran libtbb-dev
  cd /opt
  SCIP_INSTALLER="SCIPOptSuite-9.0.0-Linux-ubuntu22.sh"
  curl -O https://www.scipopt.org/download/release/$SCIP_INSTALLER
  chmod u+x $SCIP_INSTALLER
  ./$SCIP_INSTALLER --prefix=/opt/scip --exclude-subdir --skip-license
  rm $SCIP_INSTALLER
  sh -c "echo '/opt/scip/lib' > /etc/ld.so.conf.d/scip.conf"
  ldconfig

  # Install HiGHS
  echo "Installing HiGHS..."
  cd /opt
  git clone https://github.com/ERGO-Code/HiGHS.git
  cd HiGHS
  mkdir build
  cd build
  cmake -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX=/opt/HiGHS ..
  cmake --build .
  cmake --install .
  sh -c "echo '/opt/HiGHS/lib' > /etc/ld.so.conf.d/highs.conf"
  ldconfig
  cd /opt

  # Install COIN-OR CoinUtils and Osi/Clp
  echo "Installing COIN-OR CoinUtils and Osi/Clp..."
  apt-get install -y coinor-libcoinutils-dev libbz2-dev liblapack-dev libopenblas-dev
  cd /opt
  curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
  chmod u+x coinbrew
  ./coinbrew build CoinUtils --latest-release --skip-dependencies --prefix=/opt/coin-or --tests=none
  if [ $install_cplex -eq 1 ]; then
    ./coinbrew build Osi --latest-release --skip-dependencies --prefix=/opt/coin-or --tests=none --with-cplex --with-cplex-lib="-L$CPLEX_HOME/lib/x86-64_linux/static_pic -lcplex -lilocplex -lm -ldl -lpthread" --with-cplex-incdir="$CPLEX_HOME/include/ilcplex" --with-gurobi --with-gurobi-lib="-L$GUROBI_HOME/lib -lgurobi100" --with-gurobi-incdir="$GUROBI_HOME/include"
  else
    ./coinbrew build Osi --latest-release --skip-dependencies --prefix=/opt/coin-or --tests=none --without-cplex --with-gurobi --with-gurobi-lib="-L$GUROBI_HOME/lib -lgurobi100" --with-gurobi-incdir="$GUROBI_HOME/include"
  fi
  ./coinbrew build Clp --latest-release --skip-dependencies --prefix=/opt/coin-or --tests=none
  rm -R coinbrew build
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/opt/coin-or/lib"
  ldconfig

  # Install StOpt
  echo "Installing StOpt..."
  apt-get install -y zlib1g-dev
  cd /opt
  git clone https://gitlab.com/stochastic-control/StOpt
  cd StOpt
  mkdir build
  cd build
  cmake -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF -DCMAKE_INSTALL_PREFIX=/opt/StOpt ..
  cmake --build .
  cmake --install .
  cd /opt

  echo "Installation completed successfully on Ubuntu."
}

# Function to install dependencies on macOS
install_on_macos() {
  set -e # Exit immediately if a command exits with a non-zero status

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
  brew install libomp

  # Install Eigen
  echo "Installing Eigen..."
  brew install eigen

  # Install NetCDF
  echo "Installing NetCDF..."
  brew install netcdf

  # Install CPLEX
  if [ $install_cplex -eq 1 ]; then
    echo "Installing CPLEX..."
    cd /Applications
    CPLEX_INSTALLER="cplex_studio2211.osx.zip"
    curl -O https://drive.google.com/uc?id=1_xE4MBohevx3Bb_lpl8euXyYWKS_zcVK
    tar -xvf $CPLEX_INSTALLER
    # TODO
    rm $CPLEX_INSTALLER
    mv ./CPLEX_Studio2211 /Applications/CPLEX_Studio
    export CPLEX_HOME="/Applications/CPLEX_Studio"
    export PATH="${PATH}:${CPLEX_HOME}/bin/x86-64_osx"
    export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${CPLEX_HOME}/lib/x86-64_osx"
    ldconfig
  fi

  # Install Gurobi
  echo "Installing Gurobi..."
  cd /Library
  GUROBI_INSTALLER="gurobi10.0.3_macos_universal2.pkg"
  curl -O https://packages.gurobi.com/10.0/$GUROBI_INSTALLER
  installer -pkg $GUROBI_INSTALLER -target /
  rm $GUROBI_INSTALLER
  mv ./gurobi1003 /Library/gurobi
  export GUROBI_HOME="/Library/gurobi"
  export PATH="${PATH}:${GUROBI_HOME}/bin"
  export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
  ldconfig

  # Install SCIP
  echo "Installing SCIP..."
  brew install gcc tbb
  SCIP_INSTALLER="SCIPOptSuite-9.0.0-Darwin.sh"
  curl -O https://www.scipopt.org/download/release/$SCIP_INSTALLER
  chmod u+x $SCIP_INSTALLER
  ./$SCIP_INSTALLER --prefix=/Library/scip --exclude-subdir --skip-license
  rm $SCIP_INSTALLER

  # Install HiGHS
  echo "Installing HiGHS..."
  cd /Library
  git clone https://github.com/ERGO-Code/HiGHS.git
  cd HiGHS
  mkdir build
  cd build
  cmake -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX=/Library/HiGHS ..
  cmake --build .
  cmake --install .
  cd /Library

  # Install COIN-OR CoinUtils and Osi/Clp
  echo "Installing COIN-OR CoinUtils and Osi/Clp..."
  brew install coinutils bz2 lapack openblas
  cd /Library
  curl -O https://raw.githubusercontent.com/coin-or/coinbrew/master/coinbrew
  chmod u+x coinbrew
  ./coinbrew fetch CoinUtils --no-prompt
  ./coinbrew build CoinUtils --prefix=/Library/coin-or --no-prompt --tests=none
  if [ $install_cplex -eq 1 ]; then
    ./coinbrew build Osi --prefix=/Library/coin-or --no-prompt --tests=none --with-cplex --with-cplex-lib="-L$CPLEX_HOME/lib/x86-64_osx/static_pic -lcplex -lilocplex -lm -ldl -lpthread" --with-cplex-incdir="$CPLEX_HOME/include/ilcplex" --with-gurobi --with-gurobi-lib="-L$GUROBI_HOME/lib -lgurobi100" --with-gurobi-incdir="$GUROBI_HOME/include"
  else
    ./coinbrew build Osi --prefix=/Library/coin-or --no-prompt --tests=none --without-cplex --with-gurobi --with-gurobi-lib="-L$GUROBI_HOME/lib -lgurobi100" --with-gurobi-incdir="$GUROBI_HOME/include"
  fi
  ./coinbrew build Clp --prefix=/Library/coin-or --no-prompt --tests=none
  rm -R coinbrew build

  # Install StOpt
  echo "Installing StOpt..."
  brew install zlib
  cd /Library
  git clone https://gitlab.com/stochastic-control/StOpt
  cd StOpt
  mkdir build
  cd build
  cmake -DBUILD_PYTHON=OFF -DBUILD_TEST=OFF -DCMAKE_INSTALL_PREFIX=/Library/StOpt ..
  cmake --build .
  cmake --install .
  cd /Library

  echo "Installation completed successfully on macOS."
}

# Default value indicating if CPLEX should be installed
install_cplex=1

# Loop through arguments to check for the -without-cplex flag
for arg in "$@"; do
  if [ "$arg" = "-without-cplex" ]; then
    install_cplex=0
    break
  fi
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
    fi
  else
    echo "This script supports Ubuntu only."
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

# Compile SMSpp
repoPath="smspp-project"
# Check if the repo exists
if [ ! -d "$repoPath" ]; then
    echo "Repository not found locally. Cloning SMSpp..."
    git clone -b develop https://gitlab.com/smspp/smspp-project.git "$repoPath"
else
    echo "Repository found. Skipping clone."
fi
cd $repoPath

mkdir build
cd build
echo "Compiling SMSpp..."
cmake ..
# select submodules, then press c to Configure and g to Generate the build files
ccmake -DCMAKE_INSTALL_PREFIX="$CMAKE_PREFIX" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
cmake --install .
cd ..
