# RUN FROM A POWERSHELL WITH ADMINISTRATOR RIGHTS

# Default value indicating if CPLEX should be installed
param(
    [switch]$withoutCplex
)

# CMake exe path
$CMAKE_EXE = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

# Set the VCPKG_ROOT environment variable
$env:VCPKG_ROOT = "C:\vcpkg"

# Detect operating system and execute the appropriate installation function
$OS = [System.Environment]::OSVersion.Platform
if ($OS -eq "Win32NT")
{
    Set-Location "C:\"

    Write-Host "Starting the installation process on Windows..."

    # Install Visual Studio (English language pack) with the "Desktop Development with C++"
    if (-not (Test-Path "C:\Program Files\Microsoft Visual Studio"))
    {
        Write-Host "Installing Microsoft Visual Studio compiler (select `"Desktop Development with C++`")..."
        $VISUAL_STUDIO_INSTALLER = "VisualStudioSetup.exe"
        Invoke-WebRequest -Uri "https://c2rsetup.officeapps.live.com/c2r/downloadVS.aspx?sku=community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030:108d217f1e244b9aa0326ce9a131978a" -OutFile $VISUAL_STUDIO_INSTALLER
        Start-Process -FilePath $VISUAL_STUDIO_INSTALLER -Wait
        Remove-Item $VISUAL_STUDIO_INSTALLER
    }

    # Load the developer PowerShell for Visual Studio
    & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

    # Install basic requirements using Chocolatey
    Write-Host "Installing basic requirements..."
    if (-not (Test-Path "C:\ProgramData\chocolatey"))
    {
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
        refreshenv
    }
    choco install git sed

    # Install vcpkg
    Write-Host "Installing vcpkg..."
    if (-not (Test-Path $env:VCPKG_ROOT))
    {
        git clone https://github.com/microsoft/vcpkg.git $env:VCPKG_ROOT
        Set-Location $env:VCPKG_ROOT
        .\bootstrap-vcpkg.bat
    }
    else
    {
        Set-Location $env:VCPKG_ROOT
        git pull
        .\bootstrap-vcpkg.bat
        .\vcpkg upgrade --no-dry-run
    }

    # Install basic requirements with vcpkg
    Write-Host "Installing basic requirements with vcpkg..."
    .\vcpkg install zlib bzip2 pthreads getopt --triplet x64-windows

    # Install Boost libraries
    Write-Host "Installing Boost libraries..."
    .\vcpkg install boost --triplet x64-windows
    Start-Process -FilePath "$env:VCPKG_ROOT\downloads\msmpisetup-10.1.12498.exe" -Wait
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
        $CPLEX_ROOT = "C:\IBM\ILOG\CPLEX_Studio"
        if (-not (Test-Path $CPLEX_ROOT))
        {
            Set-Location "C:\"
            $CPLEX_INSTALLER = "cplex_studio2211.win_x86_64.exe"
            & wget "https://drive.google.com/uc?id=1mtjzf3id5CDh5Z5-W4D5e1z4llDw7Kta&export=download" -OutFile $CPLEX_INSTALLER
            # Invoke-WebRequest -Uri "https://drive.google.com/uc?id=1mtjzf3id5CDh5Z5-W4D5e1z4llDw7Kta&export=download" -OutFile $CPLEX_INSTALLER
            Start-Process -FilePath $CPLEX_INSTALLER -Wait
            Remove-Item $CPLEX_INSTALLER
            # Copy "IBM" folder from "C:\Program Files" to "C:\" to avoid errors due to
            # spaces in the next when building coin COIN-OR Osi with Cplex interface
            Copy-Item -Path "C:\Program Files\IBM" -Destination "C:\IBM" -Recurse
            Move-Item -Path "C:\IBM\ILOG\CPLEX_Studio2211" -Destination $CPLEX_ROOT -ErrorAction SilentlyContinue
        }
        Write-Host " done."
    }

    # Install Gurobi
    Write-Host "Installing Gurobi..." -NoNewline
    $GUROBI_ROOT = "C:\gurobi"
    if (-not (Test-Path $GUROBI_ROOT))
    {
        Set-Location "C:\"
        $GUROBI_INSTALLER = "Gurobi-10.0.3-win64.msi"
        Invoke-WebRequest -Uri "https://packages.gurobi.com/10.0/$GUROBI_INSTALLER" -OutFile $GUROBI_INSTALLER
        Start-Process -FilePath "msiexec.exe" -ArgumentList "/i", $GUROBI_INSTALLER -Wait
        Remove-Item $GUROBI_INSTALLER
        Move-Item -Path ".\gurobi1003" -Destination $GUROBI_ROOT -ErrorAction SilentlyContinue
    }
    Write-Host " done."

    # Install SCIP
    Write-Host "Installing SCIP..." -NoNewline
    $SCIP_ROOT = "C:\Program Files\SCIPOptSuite"
    if (-not (Test-Path $SCIP_ROOT))
    {
        Set-Location "C:\"
        $SCIP_INSTALLER = "SCIPOptSuite-9.0.0-win64-VS15.exe"
        Invoke-WebRequest -Uri "https://www.scipopt.org/download/release/$SCIP_INSTALLER" -OutFile $SCIP_INSTALLER
        Start-Process -FilePath $SCIP_INSTALLER -Wait
        Remove-Item $SCIP_INSTALLER
        Move-Item -Path "C:\Program Files\SCIPOptSuite 9.0.0" -Destination $SCIP_ROOT -ErrorAction SilentlyContinue
    }
    Write-Host " done."

    # Install HiGHS
    Write-Host "Installing HiGHS..."
    $HiGHS_ROOT = "C:\HiGHS"
    if (-not (Test-Path $HiGHS_ROOT))
    {
        git clone https://github.com/ERGO-Code/HiGHS.git $HiGHS_ROOT
        Set-Location $HiGHS_ROOT
        New-Item -Path "build" -ItemType Directory -Force
        Set-Location "build"
        # Build Debug
        & $CMAKE_EXE '-DFAST_BUILD=ON' '-DCMAKE_INSTALL_PREFIX=C:\HiGHS' '-DCMAKE_BUILD_TYPE=Debug' "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" '..'
        & $CMAKE_EXE '--build' '.' '--config' 'Debug'
        & $CMAKE_EXE '--install' '.'
        # Build Release
        & $CMAKE_EXE '-DFAST_BUILD=ON' '-DCMAKE_INSTALL_PREFIX=C:\HiGHS' '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" '..'
        & $CMAKE_EXE '--build' '.' '--config' 'Release'
        & $CMAKE_EXE '--install' '.'
        Set-Location "C:\"
    }

    # Install COIN-OR CoinUtils
    Write-Host "Installing COIN-OR CoinUtils..."
    Set-Location $env:VCPKG_ROOT
    .\vcpkg install coinutils blas lapack --triplet x64-windows

    Set-Location "$env:VCPKG_ROOT\ports\coin-or-osi"

    # Backup the original portfile.cmake
    Copy-Item -Path "portfile.cmake" -Destination "portfile.cmake.bak"

    Write-Host "Modifying COIN-OR Osi portfile.cmake for Gurobi interface..."

    # Use sed `/old/c\new` to replace the configuration line
    sed -i '/--without-gurobi/c\
          --with-gurobi\
          --with-gurobi-lib=C:\\\/gurobi\\\/win64\\\/lib\\\/gurobi100.lib\
          --with-gurobi-incdir=C:\\\/gurobi\\\/win64\\\/include\
          --with-gurobi-cflags=-IC:\\\/gurobi\\\/win64\\\/include\
          --with-gurobi-lflags=C:\\\/gurobi\\\/win64\\\/lib\\\/gurobi100.lib' portfile.cmake

    Write-Host "COIN-OR Osi portfile modified for Gurobi interface."

    if (-not $withoutCplex)
    {
        Write-Host "Modifying COIN-OR Osi portfile.cmake for CPLEX interface..."

        # Use sed `/old/c\new` to replace the configuration line
        sed -i '/--without-cplex/c\
            --with-cplex\
            --with-cplex-lib=C:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/lib\\\/x64_windows_msvc14\\\/stat_mda\\\/cplex2211.lib\
            --with-cplex-incdir=C:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/include\\\/ilcplex\
            --with-cplex-cflags=-IC:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/include\\\/ilcplex\
            --with-cplex-lflags=C:\\\/IBM\\\/ILOG\\\/CPLEX_Studio\\\/cplex\\\/lib\\\/x64_windows_msvc14\\\/stat_mda\\\/cplex2211.lib' portfile.cmake

        Write-Host "COIN-OR Osi portfile modified for CPLEX interface."
    }

    # Install COIN-OR Osi/Clp
    Write-Host "Installing COIN-OR Osi/Clp..."
    Set-Location $env:VCPKG_ROOT
    .\vcpkg install coin-or-osi coin-or-clp glpk --triplet x64-windows

    # Setup vcpkg for StOpt installation
    Write-Host "Setting up vcpkg for StOpt installation..."
    Set-Location "C:\"
    git clone https://gitlab.com/stochastic-control/vcpkg-registry
    Set-Location $env:VCPKG_ROOT
    .\vcpkg install stopt --overlay-ports=C:\vcpkg-registry\ports\stopt --triplet x64-windows
    Remove-Item -Path "C:\vcpkg-registry" -Recurse -Force
    Set-Location "C:\"

    Write-Host "Installation completed successfully on Windows."
}
else
{
    Write-Host "This script does not support the detected operating system."
    exit 1
}

# Install SMSPP
Write-Host "Compiling SMSpp..."
$SMSPP_ROOT = "C:\smspp-project"

if (-not (Test-Path $SMSPP_ROOT))
{
    git clone -b develop --recurse-submodules https://gitlab.com/smspp/smspp-project.git $SMSPP_ROOT
    Set-Location $SMSPP_ROOT
}
else
{
    Set-Location $SMSPP_ROOT
    git pull
}

New-Item -Path "build" -ItemType Directory -Force
Set-Location "build"
# Build Debug
& $CMAKE_EXE "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT" '-DCMAKE_BUILD_TYPE=Debug' "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" '-Wno-dev' '..'
& $CMAKE_EXE '--build' '.' '--config' 'Debug'
& $CMAKE_EXE '--install' '.'
# Build Release
& $CMAKE_EXE "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT" '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" '-Wno-dev' '..'
& $CMAKE_EXE '--build' '.' '--config' 'Release'
& $CMAKE_EXE '--install' '.'
Set-Location ".."
