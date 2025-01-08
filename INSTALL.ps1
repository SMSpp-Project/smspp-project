<#
    .SYNOPSIS
    This script installs SMS++ and all its dependencies.

    .DESCRIPTION
    This script performs the installation of SMS++ and all its dependencies.
    If not already present, it clones the smspp-project repositories, then builds and installs them.

    You can use the `-withoutCplex` option to skip the installation of CPLEX.
    You can use the `-withoutGurobi` option to skip the installation of Gurobi.

    .AUTHOR
    Donato Meoli

    .NOTES
    Ensure that you run this script using PowerShell as administrator.

    If you encounter an error about script execution policies, use the following command to temporarily allow
    script execution for the current session:

        Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force

    otherwise, you can modify the script execution policy overall in the system by:

        Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope LocalMachine -Force

    .EXAMPLES
    If you are inside the cloned repository:

        .\INSTALL.ps1

    or:

        .\INSTALL.ps1 -withoutCplex
    if you do not have a CPLEX license.

        .\INSTALL.ps1 -withoutGurobi
    if you do not have a Gurobi license.

    If you have not yet cloned the SMS++ repository, you can run the script directly:

        & ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1')))

    or:

        & ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1'))) -withoutCplex
    if you do not have a CPLEX license.

        & ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1'))) -withoutGurobi
    if you do not have a Gurobi license.
#>

# Default value indicating if CPLEX should be installed
param(
    [switch]$withoutCplex,
    [switch]$withoutGurobi
)

# Set the VCPKG_ROOT environment variable
$env:VCPKG_ROOT = "C:\vcpkg"

$STOPT_VCPKG_REGISTRY = "C:\vcpkg-registry"

function Update-EnvironmentVariables
{
    param (
        [string]$oldPattern,
        [string]$newValue
    )

    # Escape the old pattern for regex use
    $escapedPattern = [regex]::Escape($oldPattern)

    # Get all environment variables
    $envVars = [System.Environment]::GetEnvironmentVariables([System.EnvironmentVariableTarget]::Machine)

    # Iterate over each environment variable
    foreach ($envVar in $envVars.GetEnumerator())
    {
        $envVarName = $envVar.Key
        $envVarValue = $envVar.Value

        # Check if the environment variable value contains the old pattern
        if ($envVarValue -match $escapedPattern)
        {
            # Replace the old pattern with the new value
            $newEnvVarValue = $envVarValue -replace $escapedPattern, $newValue
            # Update the environment variable
            [System.Environment]::SetEnvironmentVariable($envVarName, $newEnvVarValue, [System.EnvironmentVariableTarget]::Machine)
            Write-Host "Updated $envVarName"
        }
    }
    Write-Host "All relevant environment variables have been updated."
}

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
        $VISUAL_STUDIO_INSTALLER = "C:\VisualStudioSetup.exe"
        Invoke-WebRequest -Uri "https://c2rsetup.officeapps.live.com/c2r/downloadVS.aspx?sku=community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030:108d217f1e244b9aa0326ce9a131978a" -OutFile $VISUAL_STUDIO_INSTALLER
        Start-Process -FilePath $VISUAL_STUDIO_INSTALLER -Wait
        Remove-Item $VISUAL_STUDIO_INSTALLER
    }

    # Load the developer PowerShell for Visual Studio
    & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

    # Install basic requirements using Chocolatey
    Write-Host "Installing basic requirements..."
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
    if (-not (Test-Path "C:\ProgramData\chocolatey"))
    {
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
        refreshenv
    }
    choco install git sed -y
    choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
    Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
    refreshenv

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
        if (.\vcpkg list | Select-String -Pattern "^stopt\b") # stopt is installed
        {
            Set-Location $STOPT_VCPKG_REGISTRY
            git remote update
            $local = git rev-parse "@"
            $remote = git rev-parse "@{u}"
            if ($local -eq $remote) # stopt is latest
            {
                git pull
                Set-Location $env:VCPKG_ROOT
                # upgrade all other packages ignoring stopt
                .\vcpkg list | ForEach-Object {
                    $package = ($_ -split '\s+')[0] # first column
                    if ($package -notlike "*stopt*" -and $package -notmatch '\[.*\]')
                    {
                        .\vcpkg upgrade $package --no-dry-run
                    }
                }
            }
            else # new stopt version is available
            {
                Set-Location $env:VCPKG_ROOT
                .\vcpkg remove stopt # remove the old stopt version before upgrade
                .\vcpkg upgrade --no-dry-run # upgrade all other packages
                .\vcpkg install stopt --overlay-ports=$STOPT_VCPKG_REGISTRY\ports\stopt --triplet x64-windows # install the new stopt version
            }
        }
        else
        {
            .\vcpkg upgrade --no-dry-run
        }
    }

    # Install basic requirements with vcpkg
    Write-Host "Installing basic requirements with vcpkg..."
    .\vcpkg install zlib bzip2 pthreads getopt --triplet x64-windows

    # Install Boost libraries
    Write-Host "Installing Boost libraries..."
    .\vcpkg install boost --triplet x64-windows
    if (-not (Test-Path "C:\Program Files\Microsoft MPI"))
    {
        Start-Process -FilePath "$env:VCPKG_ROOT\downloads\msmpisetup-10.1.12498.exe" -Wait
    }
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
            $CPLEX_INSTALLER = "C:\cplex_studio2211.win_x86_64.exe"
            # the CPLEX_URL is always given by the same prefix, i.e.:
            # "https://drive.usercontent.google.com/download?id=" +
            # the id code suffix in the Drive sharing link, i.e.:
            # https://drive.google.com/file/d/ 1mtjzf3id5CDh5Z5-W4D5e1z4llDw7Kta /view?usp=sharing
            $CPLEX_URL = "https://drive.usercontent.google.com/download?id=1mtjzf3id5CDh5Z5-W4D5e1z4llDw7Kta"
            if ((Invoke-WebRequest -Uri $CPLEX_URL -SessionVariable session).Content -match 'name="uuid" value="([^"]+)"')
            {
                Start-BitsTransfer -Source "$CPLEX_URL&export=download&authuser=0&confirm=t&uuid=$matches[1]" -Destination $CPLEX_INSTALLER
                Start-Process -FilePath $CPLEX_INSTALLER -Wait
                Remove-Item $CPLEX_INSTALLER
                # Move "IBM" folder from "C:\Program Files" to "C:\" to avoid errors due to
                # spaces in the next when building COIN-OR Osi with Cplex interface
                Move-Item -Path "C:\Program Files\IBM" -Destination "C:\IBM"
                Move-Item -Path "C:\IBM\ILOG\CPLEX_Studio2211" -Destination $CPLEX_ROOT -ErrorAction SilentlyContinue
                # Update the system PATH to ensure the SMS++ exe can correctly locate the cplex*.dll file
                Update-EnvironmentVariables -oldPattern "C:\Program Files\IBM\ILOG\CPLEX_Studio2211" -newValue $CPLEX_ROOT
            }
            else
            {
                Write-Host "Error: unable to find the UUID value in the response. The CPLEX download link could not be constructed."
                exit 1
            }
        }
        Write-Host " done."
    }

    # Install Gurobi if necessary
    if (-not $withoutGurobi)
    {
        Write-Host "Installing Gurobi..." -NoNewline
        $GUROBI_ROOT = "C:\gurobi"
        if (-not (Test-Path $GUROBI_ROOT))
        {
            Set-Location "C:\"
            $GUROBI_INSTALLER = "Gurobi-10.0.3-win64.msi"
            Invoke-WebRequest -Uri "https://packages.gurobi.com/10.0/$GUROBI_INSTALLER" -OutFile "C:\$GUROBI_INSTALLER"
            Start-Process -FilePath "msiexec.exe" -ArgumentList "/i", "C:\$GUROBI_INSTALLER" -Wait
            Remove-Item "C:\$GUROBI_INSTALLER"
            Move-Item -Path ".\gurobi1003" -Destination $GUROBI_ROOT -ErrorAction SilentlyContinue
            # Update the system PATH to ensure the SMS++ exe can correctly locate the gurobi*.dll file
            Update-EnvironmentVariables -oldPattern "C:\gurobi1003" -newValue $GUROBI_ROOT
        }
        Write-Host " done."
    }

    # Install SCIP
    Write-Host "Installing SCIP..." -NoNewline
    $SCIP_ROOT = "C:\Program Files\SCIPOptSuite"
    if (-not (Test-Path $SCIP_ROOT))
    {
        Set-Location "C:\"
        $SCIP_INSTALLER = "SCIPOptSuite-9.0.0-win64-VS15.exe"
        Invoke-WebRequest -Uri "https://www.scipopt.org/download/release/$SCIP_INSTALLER" -OutFile "C:\$SCIP_INSTALLER"
        Start-Process -FilePath "C:\$SCIP_INSTALLER" -Wait
        Remove-Item "C:\$SCIP_INSTALLER"
        Move-Item -Path "C:\Program Files\SCIPOptSuite 9.0.0" -Destination $SCIP_ROOT -ErrorAction SilentlyContinue
        # Update the system PATH to ensure the SMS++ exe can correctly locate the scip*.dll file
        Update-EnvironmentVariables -oldPattern "C:\Program Files\SCIPOptSuite 9.0.0" -newValue $SCIP_ROOT
    }
    Write-Host " done."

    # Install HiGHS
    Write-Host "Installing HiGHS..." -NoNewline
    $HiGHS_ROOT = "C:\HiGHS"
    if (-not (Test-Path $HiGHS_ROOT))
    {
        Write-Host "" # new line
        git clone https://github.com/ERGO-Code/HiGHS.git $HiGHS_ROOT
        Set-Location $HiGHS_ROOT
        git checkout v1.6.0 # TODO remove in the future when the "fatal error LNK1241: linker generated manifest res" will be fix
        # Build Debug
        & cmake -S . -B 'build' -G 'Visual Studio 17 2022' `
                '-DFAST_BUILD=ON' `
                "-DCMAKE_INSTALL_PREFIX=$HiGHS_ROOT" `
                '-DCMAKE_BUILD_TYPE=Debug' `
                "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
        & cmake '--build' 'build' '--config' 'Debug'
        & cmake '--install' 'build' '--config' 'Debug'
        # Build Release
        & cmake -S . -B 'build' -G 'Visual Studio 17 2022' `
                '-DFAST_BUILD=ON' `
                "-DCMAKE_INSTALL_PREFIX=$HiGHS_ROOT" `
                '-DCMAKE_BUILD_TYPE=Release' `
                "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
        & cmake '--build' 'build' '--config' 'Release'
        & cmake '--install' 'build' '--config' 'Release'
        # Define the possible paths
        $debugPath1 = "$HiGHS_ROOT\build\DEBUG\bin"; $debugPath2 = "$HiGHS_ROOT\build\bin\Debug"
        $releasePath1 = "$HiGHS_ROOT\build\RELEASE\bin"; $releasePath2 = "$HiGHS_ROOT\build\bin\Release"
        # Use an inline if-like construct to assign the paths with error handling
        $debugPath = if (Test-Path $debugPath1) { $debugPath1 }
                     elseif (Test-Path $debugPath2) { $debugPath2 }
                     else { Write-Host "No valid path found for HiGHS Debug"; exit 1 }
        $releasePath = if (Test-Path $releasePath1) { $releasePath1 }
                       elseif (Test-Path $releasePath2) { $releasePath2 }
                       else { Write-Host "No valid path found for HiGHS Release"; exit 1 }
        # Check if paths are already in the current Path
        if ($env:Path -notcontains $releasePath)
        {
            [System.Environment]::SetEnvironmentVariable("Path", $env:Path + ";$releasePath", [System.EnvironmentVariableTarget]::Machine)
        }
        if ($env:Path -notcontains $debugPath)
        {
            [System.Environment]::SetEnvironmentVariable("Path", $env:Path + ";$debugPath", [System.EnvironmentVariableTarget]::Machine)
        }
        if ($env:Path -notcontains $binPath)
        {
            [System.Environment]::SetEnvironmentVariable("Path", $env:Path + ";$binPath", [System.EnvironmentVariableTarget]::Machine)
        }
        Write-Host "Highs Paths added to the Path"
    }
    else
    {
        Write-Host " done." # TODO remove in the future when the "fatal error LNK1241: linker generated manifest res" will be fix and uncomment the following code
        <#Set-Location $HiGHS_ROOT
        git remote update
        $local = git rev-parse "@"
        $remote = git rev-parse "@{u}"
        if ($local -ne $remote) # HiGHS is not latest
        {
            git pull
            Write-Host "" # new line
            # Build Debug
            & cmake -S . -B 'build' -G 'Visual Studio 17 2022' `
                    '-DFAST_BUILD=ON' `
                    "-DCMAKE_INSTALL_PREFIX=$HiGHS_ROOT" `
                    '-DCMAKE_BUILD_TYPE=Debug' `
                    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
            & cmake '--build' 'build' '--config' 'Debug'
            & cmake '--install' 'build' '--config' 'Debug'
            # Build Release
            & cmake -S . -B 'build' -G 'Visual Studio 17 2022' `
                    '-DFAST_BUILD=ON' `
                    "-DCMAKE_INSTALL_PREFIX=$HiGHS_ROOT" `
                    '-DCMAKE_BUILD_TYPE=Release' `
                    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
            & cmake '--build' 'build' '--config' 'Release'
            & cmake '--install' 'build' '--config' 'Release'
        }
        else
        {
            Write-Host " done."
        }#>
    }
    Set-Location "C:\"

    # Install COIN-OR CoinUtils
    Write-Host "Installing COIN-OR CoinUtils..."
    Set-Location $env:VCPKG_ROOT
    .\vcpkg install coinutils blas lapack --triplet x64-windows

    if (-not $withoutGurobi)
    {
        Write-Host "Modifying COIN-OR Osi portfile.cmake for Gurobi interface..."

        Set-Location "$env:VCPKG_ROOT\ports\coin-or-osi"

        # Backup the original portfile.cmake
        #Copy-Item -Path "portfile.cmake" -Destination "portfile.cmake.bak"

        # Use sed `/old/c\new` to replace the configuration line
        sed -i '/--without-gurobi/c\
          --with-gurobi\
          --with-gurobi-lib=C:\\\/gurobi\\\/win64\\\/lib\\\/gurobi100.lib\
          --with-gurobi-incdir=C:\\\/gurobi\\\/win64\\\/include\
          --with-gurobi-cflags=-IC:\\\/gurobi\\\/win64\\\/include\
          --with-gurobi-lflags=C:\\\/gurobi\\\/win64\\\/lib\\\/gurobi100.lib' portfile.cmake

        Write-Host "COIN-OR Osi portfile modified for Gurobi interface."
    }

    if (-not $withoutCplex)
    {
        Write-Host "Modifying COIN-OR Osi portfile.cmake for CPLEX interface..."

        Set-Location "$env:VCPKG_ROOT\ports\coin-or-osi"

        # Backup the original portfile.cmake
        #Copy-Item -Path "portfile.cmake" -Destination "portfile.cmake.bak"

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
    git clone https://gitlab.com/stochastic-control/vcpkg-registry.git
    Set-Location $env:VCPKG_ROOT
    .\vcpkg install stopt --overlay-ports=$STOPT_VCPKG_REGISTRY\ports\stopt --triplet x64-windows
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

# Check if the SMSpp repository already exists
if (Test-Path $SMSPP_ROOT)
{
    Set-Location $SMSPP_ROOT
    Write-Host "SMSpp already exists. Pulling latest changes..."
    git pull
}
else
{
    Write-Host "Repository not found locally. Cloning SMSpp..."
    git clone --branch develop https://gitlab.com/smspp/smspp-project.git $SMSPP_ROOT
    Set-Location $SMSPP_ROOT
}

# Build SMSpp Debug
& cmake -S . -B 'cmake-build-debug' -G 'Visual Studio 17 2022' `
        "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT/Debug" `
        '-DCMAKE_BUILD_TYPE=Debug' `
        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
        '-Wno-dev'
# run cmake-gui
Start-Process -FilePath "cmake-gui" -ArgumentList "cmake-build-debug" -Wait # select submodules, then Configure and Generate the build files
& cmake '--build' 'cmake-build-debug' '--config' 'Debug'
& cmake '--install' 'cmake-build-debug' '--config' 'Debug'
#Set-Location "cmake-build-debug"
#& ctest -V -C Debug
#Set-Location $SMSPP_ROOT

# Build SMSpp Release
& cmake -S . -B 'cmake-build-release' -G 'Visual Studio 17 2022' `
        "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT/Release" `
        '-DCMAKE_BUILD_TYPE=Release' `
        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
        '-Wno-dev'
# run cmake-gui
Start-Process -FilePath "cmake-gui" -ArgumentList "cmake-build-release" -Wait # select submodules, then Configure and Generate the build files
& cmake '--build' 'cmake-build-release' '--config' 'Release'
& cmake '--install' 'cmake-build-release' '--config' 'Release'
#Set-Location "cmake-build-release"
#& ctest -V -C Release
#Set-Location $SMSPP_ROOT
