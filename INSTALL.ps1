<#
    .SYNOPSIS
    This script installs SMS++ and all its dependencies.

    .DESCRIPTION
    This script performs the installation of SMS++ and all its dependencies.
    If not already present, it clones the smspp-project repositories, then builds and installs them.

    You can use the `-installRoot <your-custom-path>` option to specify your SMS++ custom installation root.
    You can use the `-withoutCplex` option to skip the installation of CPLEX.
    You can use the `-withoutGurobi` option to skip the installation of Gurobi.
    You can use the `-withoutSCIP` option to skip the installation of SCIP.
    You can use the `-withoutHiGHS` option to skip the installation of HiGHS.
    You can use the `-withoutStOpt` option to skip the installation of StOpt.
    You can use the `-withoutCoinOr` option to skip the installation of COIN-OR.
    You can use the `-withoutSMSpp` option to skip the installation of SMS++.

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

        .\INSTALL.ps1 -installRoot <your-custom-path> without<some-dependency>

    If you have not yet cloned the SMS++ repository, you can run the script directly:

        & ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1'))) -installRoot <your-custom-path> -without<some-dependency>

#>

# Default values indicating if dependencies should be installed
param(
    [switch]$withoutCplex,
    [switch]$withoutGurobi,
    [switch]$withoutSCIP,
    [switch]$withoutHiGHS,
    [switch]$withoutStOpt,
    [switch]$withoutCoinOr,
    [switch]$withoutSMSpp,
    [switch]$nonInteractive,
    [string]$installRoot = "C:\" # Default if not provided
)

# Remove trailing backslash from installRoot if present
$installRoot = $installRoot.TrimEnd('\')

# Default value for the maximum number of jobs is the number of logical processors if not already defined
$MAX_JOBS = $env:MAX_JOBS
if (-not $MAX_JOBS) {
    $MAX_JOBS = (Get-CimInstance Win32_Processor | Measure-Object -Property NumberOfLogicalProcessors -Sum).Sum
}

# Set the VCPKG_ROOT environment variable
$env:VCPKG_ROOT = "C:\vcpkg"

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
    foreach ($envVar in $envVars.GetEnumerator()) {
        $envVarName = $envVar.Key
        $envVarValue = $envVar.Value

        # Check if the environment variable value contains the old pattern
        if ($envVarValue -match $escapedPattern) {
            # Replace the old pattern with the new value
            $newEnvVarValue = $envVarValue -replace $escapedPattern, $newValue
            # Update the environment variable
            [System.Environment]::SetEnvironmentVariable($envVarName, $newEnvVarValue, [System.EnvironmentVariableTarget]::Machine)
            Write-Host "Updated $envVarName in the system Path."
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

    # Attempt to locate an existing Visual Studio installation using vswhere (must have C++ tools)
    if (-not (Get-Command vswhere -ErrorAction SilentlyContinue)) {
        Write-Error "vswhere is not available in PATH. Cannot detect Visual Studio installation."
        exit 1
    }

    $vsInstallPath = vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

    if ($vsInstallPath -and (Test-Path "$vsInstallPath\Common7\Tools\VsDevCmd.bat")) {
        Write-Host "Using existing Visual Studio installation at $vsInstallPath"
        & "$vsInstallPath\Common7\Tools\VsDevCmd.bat"
    } else {
        Write-Host "No suitable Visual Studio installation found. Installing Community Edition with C++ tools..."

        $VISUAL_STUDIO_INSTALLER = "C:\VisualStudioSetup.exe"
        Invoke-WebRequest -Uri "https://c2rsetup.officeapps.live.com/c2r/downloadVS.aspx?sku=community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030:108d217f1e244b9aa0326ce9a131978a" -OutFile $VISUAL_STUDIO_INSTALLER
        Start-Process -FilePath $VISUAL_STUDIO_INSTALLER -Wait
        Remove-Item $VISUAL_STUDIO_INSTALLER

        # Re-check installation path after install
        $vsInstallPath = vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsInstallPath -and (Test-Path "$vsInstallPath\Common7\Tools\VsDevCmd.bat")) {
            Write-Host "Visual Studio successfully installed at $vsInstallPath"
            & "$vsInstallPath\Common7\Tools\VsDevCmd.bat"
        } else {
            Write-Error "Failed to install or detect Visual Studio with required components."
            exit 1
        }
    }

    # Install basic requirements using Chocolatey
    Write-Host "Installing basic requirements..."
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
    if (-not (Test-Path "C:\ProgramData\chocolatey")) {
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
        refreshenv
    }
    choco feature disable -n=showDownloadProgress
    choco install git wget -y --limit-output
    choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
    Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
    refreshenv

    # Install vcpkg
    Write-Host "Installing vcpkg..."
    if (-not (Test-Path $env:VCPKG_ROOT)) {
        git clone https://github.com/microsoft/vcpkg.git $env:VCPKG_ROOT
        Set-Location $env:VCPKG_ROOT
        .\bootstrap-vcpkg.bat
    } else {
        Set-Location $env:VCPKG_ROOT
        git pull
        .\bootstrap-vcpkg.bat
        .\vcpkg upgrade --no-dry-run
    }
    $env:VCPKG_FEATURE_FLAGS = 'manifests,registries'

    # Install CPLEX
    if (-not $withoutCplex) {
        Write-Host "Installing CPLEX..." -NoNewline
        $CPLEX_ROOT = "C:\IBM\ILOG\CPLEX_Studio"
        if (-not (Test-Path $CPLEX_ROOT)) {
            Set-Location "C:\"
            $CPLEX_INSTALLER = "C:\cplex_studio2211.win_x86_64.exe"
            # the CPLEX_URL is always given by the same prefix, i.e.:
            # "https://drive.usercontent.google.com/download?id=" +
            # the id code suffix in the Drive sharing link, i.e.:
            # https://drive.google.com/file/d/ 1mtjzf3id5CDh5Z5-W4D5e1z4llDw7Kta /view?usp=sharing
            $CPLEX_URL = "https://drive.usercontent.google.com/download?id=1mtjzf3id5CDh5Z5-W4D5e1z4llDw7Kta"
            if ((Invoke-WebRequest -Uri $CPLEX_URL -SessionVariable session).Content -match 'name="uuid" value="([^"]+)"') {
                Start-BitsTransfer -Source "$CPLEX_URL&export=download&authuser=0&confirm=t&uuid=$matches[1]" -Destination $CPLEX_INSTALLER
                Start-Process -FilePath $CPLEX_INSTALLER -Wait
                Remove-Item $CPLEX_INSTALLER
                # Move "IBM" folder from "C:\Program Files" to "C:\" to avoid errors due to
                # spaces in the next when building COIN-OR Osi with Cplex interface
                Move-Item -Path "C:\Program Files\IBM" -Destination "C:\IBM"
                Move-Item -Path "C:\IBM\ILOG\CPLEX_Studio2211" -Destination $CPLEX_ROOT -ErrorAction SilentlyContinue
                # Update the system PATH to ensure the SMS++ exe can correctly locate the cplex*.dll file
                Update-EnvironmentVariables -oldPattern "C:\Program Files\IBM\ILOG\CPLEX_Studio2211" -newValue $CPLEX_ROOT
            } else {
                Write-Host "Error: unable to find the UUID value in the response. The CPLEX download link could not be constructed."
                exit 1
            }
        }
        Write-Host " done."
    }

    # Install Gurobi
    if (-not $withoutGurobi) {
        Write-Host "Installing Gurobi..." -NoNewline
        $GUROBI_ROOT = "C:\gurobi"
        if (-not (Test-Path $GUROBI_ROOT)) {
            Set-Location "C:\"
            $GUROBI_INSTALLER = "Gurobi-12.0.1-win64.msi"
            Invoke-WebRequest -Uri "https://packages.gurobi.com/12.0/$GUROBI_INSTALLER" -OutFile "C:\$GUROBI_INSTALLER"
            Start-Process -FilePath "msiexec.exe" -ArgumentList "/i", "C:\$GUROBI_INSTALLER" -Wait
            Remove-Item "C:\$GUROBI_INSTALLER"
            Move-Item -Path ".\gurobi1201" -Destination $GUROBI_ROOT -ErrorAction SilentlyContinue
            # Update the system PATH to ensure the SMS++ exe can correctly locate the gurobi*.dll file
            Update-EnvironmentVariables -oldPattern "C:\gurobi1201" -newValue $GUROBI_ROOT
        }
        Write-Host " done."
    }

    # Install SCIP
    if (-not $withoutSCIP) {
        Write-Host "Installing SCIP..." -NoNewline
        $SCIP_ROOT = "C:\Program Files\SCIPOptSuite"
        if (-not (Test-Path $SCIP_ROOT)) {
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
    }

    # Install HiGHS
    if (-not $withoutHiGHS) {
        Write-Host "Installing HiGHS..." -NoNewline
        $HiGHS_ROOT = "C:\HiGHS"
        if (-not (Test-Path $HiGHS_ROOT)) {
            Write-Host "" # new line
            & "$env:VCPKG_ROOT\vcpkg.exe" install zlib --triplet x64-windows
            git clone https://github.com/ERGO-Code/HiGHS.git $HiGHS_ROOT
            Set-Location $HiGHS_ROOT
            # Configure once using multi-config
            & cmake -S . -B 'build' `
                    '-DFAST_BUILD=ON' `
                    "-DCMAKE_INSTALL_PREFIX=$HiGHS_ROOT" `
                    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
            # Build Debug
            & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
            & cmake '--install' 'build' '--config' 'Debug'
            # Build Release
            & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
            & cmake '--install' 'build' '--config' 'Release'
        } else {
            Write-Host " done."
            Set-Location $HiGHS_ROOT
            git remote update
            $local = git rev-parse "@"
            $remote = git rev-parse "@{u}"
            if ($local -ne $remote) { # HiGHS is not latest
                git pull
                Write-Host "" # new line
                # Re-configure once using multi-config
                & cmake -S . -B 'build' `
                        '-DFAST_BUILD=ON' `
                        "-DCMAKE_INSTALL_PREFIX=$HiGHS_ROOT" `
                        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
                # Build Debug
                & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
                & cmake '--install' 'build' '--config' 'Debug'
                # Build Release
                & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
                & cmake '--install' 'build' '--config' 'Release'
            } else {
                Write-Host "HiGHS already up to date."
            }
        }
        # Add HiGHS to the system PATH
        $HIGHS_BIN = "$HiGHS_ROOT\bin"
        $systemPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::Machine)
        if ($systemPath -notlike "*$HIGHS_BIN*") {
            $systemPath = "$HIGHS_BIN;$systemPath"
            [System.Environment]::SetEnvironmentVariable("Path", $systemPath, [System.EnvironmentVariableTarget]::Machine)
            Write-Host "Added HiGHS bin to the system Path"
        } else {
            Write-Host "HiGHS bin is already in the system Path."
        }
        Set-Location "C:\"
    }

    # Install StOpt
    if (-not $withoutStOpt) {
        Write-Host "Installing StOpt..." -NoNewline
        $StOpt_ROOT = "C:\StOpt"
        if (-not (Test-Path $StOpt_ROOT)) {
            Write-Host "" # new line
            # Configure vcpkg
            git clone https://gitlab.com/stochastic-control/StOpt.git $StOpt_ROOT
            Set-Location $StOpt_ROOT
            # Install Microsoft MPI
            if (-not (Test-Path "C:\Program Files\Microsoft MPI\Bin\mpiexec.exe")) {
                Write-Host "Installing Microsoft MPI..."
                $downloadsDir = Join-Path $env:VCPKG_ROOT 'downloads'
                if (-not (Test-Path $downloadsDir)) {
                    New-Item -ItemType Directory -Force -Path $downloadsDir | Out-Null
                }
                $msmpiInstaller = Join-Path $downloadsDir 'msmpisetup-10.1.12498.exe'
                if (-not (Test-Path $msmpiInstaller)) {
                    $msmpiUrl = "https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisetup.exe"
                    Invoke-WebRequest -Uri $msmpiUrl -OutFile $msmpiInstaller
                }
                Start-Process -FilePath $msmpiInstaller -ArgumentList "-unattend", "-force" -Wait
                Write-Host " done."
            }
            mv .\doc "C:\" # TODO remove when the doc bug in StOpt will be fixed
            # Configure once using multi-config
            & cmake -S . -B 'build' `
                    '-DBUILD_PYTHON=OFF' `
                    '-DBUILD_TEST=OFF' `
                    "-DCMAKE_INSTALL_PREFIX=$StOpt_ROOT" `
                    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
                    '-Wno-dev'
            # Build Debug
            & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
            & cmake '--install' 'build' '--config' 'Debug'
            # Build Release
            & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
            & cmake '--install' 'build' '--config' 'Release'
            mv "C:\doc" $StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
        } else {
            Write-Host " done."
            Set-Location $StOpt_ROOT
            git remote update
            $local = git rev-parse "@"
            $remote = git rev-parse "@{u}"
            if ($local -ne $remote) { # StOpt is not latest
                git pull
                Write-Host "" # new line
                mv .\doc "C:\" # TODO remove when the doc bug in StOpt will be fixed
                # Re-configure once using multi-config
                & cmake -S . -B 'build' `
                        '-DBUILD_PYTHON=OFF' `
                        '-DBUILD_TEST=OFF' `
                        "-DCMAKE_INSTALL_PREFIX=$StOpt_ROOT" `
                        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
                        '-Wno-dev'
                # Rebuild Debug
                & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
                & cmake '--install' 'build' '--config' 'Debug'
                # Rebuild Release
                & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
                & cmake '--install' 'build' '--config' 'Release'
                mv "C:\doc" $StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
            } else {
                Write-Host "StOpt already up to date."
            }
            Set-Location "C:\"
        }
        # Add StOpt to the system PATH
        $STOPT_BIN = "$StOpt_ROOT\bin"
        $systemPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::Machine)
        if ($systemPath -notlike "*$STOPT_BIN*") {
            $systemPath = "$STOPT_BIN;$systemPath"
            [System.Environment]::SetEnvironmentVariable("Path", $systemPath, [System.EnvironmentVariableTarget]::Machine)
            Write-Host "Added StOpt bin to the system Path."
        } else {
            Write-Host "StOpt bin is already in the system Path."
        }
        Set-Location "C:\"
    }

    Write-Host "Installation completed successfully on Windows."
}
else
{
    Write-Host "This script does not support the detected operating system."
    exit 1
}

# Install SMSPP
if (-not $withoutSMSpp)
{
    Write-Host "Compiling SMSpp..."
    $SMSPP_ROOT = "$installRoot\smspp-project"

    # Check if the SMSpp repository already exists
    if (Test-Path $SMSPP_ROOT) {
        Set-Location $SMSPP_ROOT
        Write-Host "SMSpp already exists. Pulling latest changes..."
        git pull
    } else {
        Write-Host "Repository not found locally. Cloning SMSpp..."
        if ($nonInteractive) {
            # no way to use cmake-gui interactively to choose submodules, so download all
            git clone --branch develop --recurse-submodules https://gitlab.com/smspp/smspp-project.git $SMSPP_ROOT
        } else {
            git clone --branch develop https://gitlab.com/smspp/smspp-project.git $SMSPP_ROOT
        }
        Set-Location $SMSPP_ROOT
    }

    # Update builtin-baseline in vcpkg.json to match the current vcpkg commit
    $ManifestPath = Join-Path $SMSPP_ROOT 'vcpkg.json'
    $Baseline = (& git -C $env:VCPKG_ROOT rev-parse HEAD).Trim()
    $manifestJson = Get-Content $ManifestPath -Raw | ConvertFrom-Json
    $manifestJson.'builtin-baseline' = $Baseline
    $manifestJson | ConvertTo-Json -Depth 10 | Set-Content $ManifestPath -Encoding UTF8

    # Configure COIN-OR Osi
    if (-not $withoutCoinOr) {
        Write-Host "Configuring COIN-OR Osi..."

        # Prepare an overlay port so vcpkg uses OUR modified portfile.cmake
        $OverlayRoot = Join-Path "$SMSPP_ROOT\vcpkg" 'overlays\ports'
        $OverlayPort = Join-Path $OverlayRoot 'coin-or-osi'
        New-Item -ItemType Directory -Force -Path $OverlayPort | Out-Null

        # Copy the original port as a base for our overlay
        Copy-Item -Recurse -Force (Join-Path "$SMSPP_ROOT\vcpkg" 'ports\coin-or-osi\*') $OverlayPort

        # Edit the portfile INSIDE THE OVERLAY (never touch the builtin port directly)
        $portfile = Join-Path $OverlayPort 'portfile.cmake'
        $osiText = Get-Content -Raw -Path $portfile

        if (-not $withoutGurobi) {
            Write-Host "Applying Gurobi interface changes to overlay portfile.cmake..."
            # Replace the line containing --without-gurobi with a multi-line --with-gurobi block
            $replacementGRB = @"
        --with-gurobi
        --with-gurobi-lib=C:/gurobi/win64/lib/gurobi120.lib
        --with-gurobi-incdir=C:/gurobi/win64/include
        --with-gurobi-cflags=-IC:/gurobi/win64/include
        --with-gurobi-lflags=C:/gurobi/win64/lib/gurobi120.lib
"@.Trim()
            $osiText = [regex]::Replace($osiText, '(?m)^[^\r\n]*--without-gurobi[^\r\n]*$', $replacementGRB)
            Write-Host "Overlay portfile updated for Gurobi."
        }

        if (-not $withoutCplex) {
            Write-Host "Applying CPLEX interface changes to overlay portfile.cmake..."
            # Replace the line containing --without-cplex with a multi-line --with-cplex block
            $replacementCPX = @"
        --with-cplex
        --with-cplex-lib=C:/IBM/ILOG/CPLEX_Studio/cplex/lib/x64_windows_msvc14/stat_mda/cplex2211.lib
        --with-cplex-incdir=C:/IBM/ILOG/CPLEX_Studio/cplex/include/ilcplex
        --with-cplex-cflags=-IC:/IBM/ILOG/CPLEX_Studio/cplex/include/ilcplex
        --with-cplex-lflags=C:/IBM/ILOG/CPLEX_Studio/cplex/lib/x64_windows_msvc14/stat_mda/cplex2211.lib
"@.Trim()
            $osiText = [regex]::Replace($osiText, '(?m)^[^\r\n]*--without-cplex[^\r\n]*$', $replacementCPX)
            Write-Host "Overlay portfile updated for CPLEX."
        }

        Set-Content -Path $portfile -Value $osiText -NoNewline
        Set-Location $SMSPP_ROOT
    }

    # Configure once using multi-config
    $env:VCPKG_FEATURE_FLAGS = 'manifests,registries'
    $env:VCPKG_BINARY_SOURCES = 'clear;default' # avoid stale cached binaries
    if (Test-Path $OverlayRoot) { $env:VCPKG_OVERLAY_PORTS = $OverlayRoot } # only if overlay exists
    $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    & cmake -S . -B 'build' "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT" '-Wno-dev'
    # run cmake-gui
    if (-not $nonInteractive) {
        # select submodules, then Configure and Generate the build files
        Start-Process -FilePath "cmake-gui" -ArgumentList 'build' -Wait
    }

    <## Build Debug
    & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
    & cmake '--install' 'build' '--config' 'Debug'
    #Set-Location 'build'
    #& ctest -V -C Debug
    #Set-Location $SMSPP_ROOT#>

    # Build Release
    & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
    & cmake '--install' 'build' '--config' 'Release'
    #Set-Location 'build'
    #& ctest -V -C Release
    #Set-Location $SMSPP_ROOT

    # Add SMSpp to the system PATH
    $SMSPP_BIN = "$SMSPP_ROOT\bin"
    $systemPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::Machine)
    if ($systemPath -notlike "*$SMSPP_BIN*") {
        $systemPath = "$SMSPP_BIN;$systemPath"
        [System.Environment]::SetEnvironmentVariable("Path", $systemPath, [System.EnvironmentVariableTarget]::Machine)
        Write-Host "Added SMSpp bin to the system Path."
    } else {
        Write-Host "SMSpp bin is already in the system Path."
    }
}