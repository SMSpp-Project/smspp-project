# RUN THIS SCRIPT FROM A "DEVELOPER POWERSHELL FOR VS" AS ADMINISTRATOR
# VISUAL STUDIO (WITH THE ENGLISH LANGUAGE PACK) WITH THE "DESKTOP DEVELOPMENT WITH C++" IS NEEDED

# Default value indicating if CPLEX should be installed
param(
    [switch]$withoutCplex
)

# CMake path
$cmakePath = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

# Detect operating system and execute the appropriate installation function
$OS = [System.Environment]::OSVersion.Platform
if ($OS -eq "Win32NT")
{
    Set-Location "C:\"

    Write-Host "Starting the installation process on Windows..."

    # Install basic requirements using Chocolatey
    Write-Host "Installing basic requirements..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
    choco install git sed
    Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
    refreshenv

    # Install vcpkg
    Write-Host "Installing vcpkg..."
    Set-Location "C:\"
    git clone https://github.com/microsoft/vcpkg.git
    Set-Location "C:\vcpkg"
    .\bootstrap-vcpkg.bat

    # Install basic requirements with vcpkg
    Write-Host "Installing basic requirements with vcpkg..."
    .\vcpkg install zlib bzip2 pthreads getopt --triplet x64-windows

    # Install Boost libraries
    Write-Host "Installing Boost libraries..."
    .\vcpkg install boost --triplet x64-windows
    Start-Process -FilePath "C:\vcpkg\downloads\msmpisetup-10.1.12498.exe" -Wait
    .\vcpkg install boost-mpi --triplet x64-windows

    # Install Eigen
    Write-Host "Installing Eigen..."
    .\vcpkg install eigen3 --triplet x64-windows

    # Install NetCDF
    Write-Host "Installing NetCDF..."
    .\vcpkg install netcdf-cxx4 --triplet x64-windows

    # Install CPLEX if necessary
    if (-not $withoutCplex)
    {
        Write-Host "Installing CPLEX..." -NoNewline
        $cplexPath = "C:\IBM\ILOG\CPLEX_Studio"
        if (-not (Test-Path $cplexPath))
        {
            Set-Location "C:\"
            $CPLEX_INSTALLER = "cplex_studio2211.win_x86_64.exe"
            Invoke-WebRequest -Uri "https://TODO/$CPLEX_INSTALLER" -OutFile $CPLEX_INSTALLER
            Start-Process -FilePath $CPLEX_INSTALLER -Wait
            Remove-Item $CPLEX_INSTALLER
            # Copy from Program Files to C:\ to avoid errors due to spaces
            Copy-Item -Path "C:\Program Files\IBM" -Destination "C:\IBM" -Recurse
            Move-Item -Path "C:\IBM\ILOG\CPLEX_Studio2211" -Destination $cplexPath -ErrorAction SilentlyContinue
        }
        Write-Host " done."
    }

    # Install Gurobi
    Write-Host "Installing Gurobi..." -NoNewline
    $gurobiPath = "C:\gurobi"
    if (-not (Test-Path $gurobiPath))
    {
        Set-Location "C:\"
        $GUROBI_INSTALLER = "Gurobi-10.0.3-win64.msi"
        Invoke-WebRequest -Uri "https://packages.gurobi.com/10.0/$GUROBI_INSTALLER" -OutFile $GUROBI_INSTALLER
        Start-Process -FilePath "msiexec.exe" -ArgumentList "/i", $GUROBI_INSTALLER -Wait
        Remove-Item $GUROBI_INSTALLER
        Move-Item -Path ".\gurobi1003" -Destination $gurobiPath -ErrorAction SilentlyContinue
    }
    Write-Host " done."

    # Install SCIP
    Write-Host "Installing SCIP..." -NoNewline
    $scipPath = "C:\Program Files\SCIPOptSuite"
    if (-not (Test-Path $scipPath))
    {
        Set-Location "C:\"
        $SCIP_INSTALLER = "SCIPOptSuite-9.0.0-win64-VS15.exe"
        Invoke-WebRequest -Uri "https://www.scipopt.org/download/release/$SCIP_INSTALLER" -OutFile $SCIP_INSTALLER
        Start-Process -FilePath $SCIP_INSTALLER -Wait
        Remove-Item $SCIP_INSTALLER
        Move-Item -Path "C:\Program Files\SCIPOptSuite 9.0.0" -Destination $scipPath -ErrorAction SilentlyContinue
    }
    Write-Host " done."

    # Install HiGHS
    Write-Host "Installing HiGHS..."
    $highsPath = "C:\HiGHS"
    if (-not (Test-Path $highsPath))
    {
        Set-Location "C:\"
        git clone https://github.com/ERGO-Code/HiGHS.git
        Set-Location "HiGHS"
        New-Item -Path "build" -ItemType Directory -Force
        Set-Location "build"
        $cmakePath -DFAST_BUILD=ON -DCMAKE_INSTALL_PREFIX=C:\HiGHS -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake ..
        $cmakePath --build . --config Release
        $cmakePath --install .
        Set-Location "C:\"
    }

    # Install COIN-OR CoinUtils
    Write-Host "Installing COIN-OR CoinUtils..."
    Set-Location "C:\vcpkg"
    .\vcpkg install coinutils blas lapack --triplet x64-windows

    Set-Location "C:\vcpkg\ports\coin-or-osi"

    # Backup the original portfile.cmake
    Copy-Item -Path "portfile.cmake" -Destination "portfile.cmake.bak"

    Write-Host "Modifying COIN-OR Osi portfile.cmake for Gurobi support..."

    # Use sed `/old/c\new` to replace the configuration line
    sed -i '/--without-gurobi/c\
          --with-gurobi\
          --with-gurobi-lib=C:\\\/gurobi\\\/win64\\\/lib\\\/gurobi100.lib\
          --with-gurobi-incdir=C:\\\/gurobi\\\/win64\\\/include\
          --with-gurobi-cflags=-IC:\\\/gurobi\\\/win64\\\/include\
          --with-gurobi-lflags=C:\\\/gurobi\\\/win64\\\/lib\\\/gurobi100.lib' portfile.cmake

    Write-Host "COIN-OR Osi portfile modified for Gurobi support."

    if (-not $withoutCplex)
    {
        Write-Host "Modifying COIN-OR Osi portfile.cmake for CPLEX support..."

        # Use sed `/old/c\new` to replace the configuration line
        sed -i '/--without-cplex/c\
            --with-cplex\
            --with-cplex-lib=C:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/lib\\\/x64_windows_msvc14\\\/stat_mda\\\/cplex2211.lib\
            --with-cplex-incdir=C:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/include\\\/ilcplex\
            --with-cplex-cflags=-IC:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/include\\\/ilcplex\
            --with-cplex-lflags=C:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/lib\\\/x64_windows_msvc14\\\/stat_mda\\\/cplex2211.lib' portfile.cmake

        Write-Host "COIN-OR Osi portfile modified for CPLEX support."
    }

    # Install COIN-OR Osi/Clp
    Write-Host "Installing COIN-OR Osi/Clp..."
    Set-Location "C:\vcpkg"
    .\vcpkg install coin-or-osi coin-or-clp glpk --triplet x64-windows

    # Setup vcpkg for StOpt installation
    Write-Host "Setting up vcpkg for StOpt installation..."
    Set-Location "C:\"
    git clone https://gitlab.com/stochastic-control/vcpkg-registry
    Set-Location "C:\vcpkg"
    .\vcpkg install stopt --overlay-ports=C:\vcpkg-registry\ports\stopt --triplet x64-windows
    Remove-Item -Path "C:\vcpkg-registry" -Recurse
    Set-Location "C:\"

    Write-Host "Installation completed successfully on Windows."
}
else
{
    Write-Host "This script does not support the detected operating system."
    exit 1
}

# Compile SMSpp
$repoPath = "smspp-project"
# Check if the repo exists
if (-not (Test-Path $repoPath))
{
    Write-Host "Repository not found locally. Cloning SMSpp..."
    git clone -b develop --recurse-submodules https://gitlab.com/smspp/smspp-project.git $repoPath
}
else
{
    Write-Host "Repository found. Skipping clone."
}
Set-Location $repoPath

New-Item -Path "build" -ItemType Directory -Force
Set-Location "build"
$cmakePath -DCMAKE_INSTALL_PREFIX=C:\SMSpp -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -Wno-dev ..
Write-Host "Compiling SMSpp..."
$cmakePath --build . --config Release
$cmakePath --install .
Set-Location ".."
