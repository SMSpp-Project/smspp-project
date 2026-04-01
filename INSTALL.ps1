<#
    .SYNOPSIS
    This script installs SMS++ and all its dependencies.

    .DESCRIPTION
    This script performs the installation of SMS++ and all its dependencies.
    If not already present, it clones the smspp-project repositories, then builds and installs them.

    You can use the `-installRoot <your-custom-path>` option to specify your SMS++ custom installation root.
    You can use the `-updatevcpkg` option to update the builtin-baseline in vcpkg.json.
    You can use the `-withoutCplex` option to skip the installation of CPLEX.
    You can use the `-withoutGurobi` option to skip the installation of Gurobi.
    You can use the `-withoutSCIP` option to skip the installation of SCIP.
    You can use the `-withoutStOpt` option to skip the installation of StOpt.
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
    [switch]$withoutStOpt,
    [switch]$withoutSMSpp,
    [switch]$updatevcpkg,
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

function Add-ToSystemPath
{
    param(
        [Parameter(Mandatory=$true)][string]$PathToAdd
    )

    if (-not (Test-Path $PathToAdd)) {
        Write-Host "Path not found, skipping: $PathToAdd"
        return
    }

    $systemPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::Machine)

    if ($systemPath -notlike "*$PathToAdd*") {
        $systemPath = "$PathToAdd;$systemPath"
        [System.Environment]::SetEnvironmentVariable("Path", $systemPath, [System.EnvironmentVariableTarget]::Machine)
        Write-Host "Added to system Path: $PathToAdd"
    } else {
        Write-Host "Already in system Path: $PathToAdd"
    }
}

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

function Get-MsMpiInstalledInfo {
    $uninstallRoots = @(
        "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*",
        "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*"
    )

    $items = foreach ($root in $uninstallRoots) {
        Get-ItemProperty $root -ErrorAction SilentlyContinue |
                Where-Object {
                    $_.DisplayName -match 'Microsoft MPI' -or $_.DisplayName -match 'MS-MPI'
                }
    }

    # Prefer runtime / redistributable if present, otherwise take the first match
    $runtime = $items |
            Where-Object { $_.DisplayName -match 'Redistributable|Runtime' } |
            Sort-Object DisplayVersion -Descending |
            Select-Object -First 1
    if ($runtime) { return $runtime }
    return ($items | Select-Object -First 1)
}

function Uninstall-ProgramSilent([string]$uninstallString) {
    if (-not $uninstallString) { return $false }

    # Typical values:
    #   MsiExec.exe /I{GUID}  or  MsiExec.exe /X{GUID}
    #   "C:\...\uninstall.exe" /uninstall ...
    $cmd = $uninstallString.Trim()

    if ($cmd -match 'MsiExec\.exe') {
        # Ensure we are uninstalling (use /X) and make it quiet
        $cmd = [regex]::Replace($cmd, '(?i)/i\{', '/X{')
        $cmd = [regex]::Replace($cmd, '(?i)\s/([i])\s', ' /X ')

        if ($cmd -notmatch '/X') {
            # fallback: if it doesn't contain /I or /X, leave it as-is
        }

        # Append quiet flags if not present
        if ($cmd -notmatch '/qn') { $cmd += ' /qn' }
        if ($cmd -notmatch 'REBOOT=ReallySuppress') { $cmd += ' REBOOT=ReallySuppress' }

        Start-Process -FilePath "cmd.exe" -ArgumentList "/c $cmd" -Wait -NoNewWindow
        return $true
    }

    # Non-MSI uninstaller: try to run it quietly
    Start-Process -FilePath "cmd.exe" -ArgumentList "/c $cmd /quiet /norestart" -Wait -NoNewWindow
    return $true
}

function Ensure-MsMpiVersion {
    param(
        [Parameter(Mandatory=$true)][string]$ExpectedVersion,
        [Parameter(Mandatory=$true)][string]$VcpkgDownloadsDir
    )

    $installed = Get-MsMpiInstalledInfo
    $installedVersion = if ($installed) { $installed.DisplayVersion } else { $null }

    if ($installed -and $installedVersion -eq $ExpectedVersion) {
        Write-Host "Microsoft MPI already installed with expected version $ExpectedVersion."
        return
    }

    if ($installed) {
        Write-Host "Microsoft MPI installed version is $installedVersion, expected $ExpectedVersion. Uninstalling..."
        $uninstallCmd = $installed.QuietUninstallString
        if (-not $uninstallCmd) { $uninstallCmd = $installed.UninstallString }

        $ok = Uninstall-ProgramSilent -uninstallString $uninstallCmd
        if (-not $ok) {
            throw "Could not determine a working uninstall command for Microsoft MPI. Uninstall manually and rerun."
        }
        Write-Host "Uninstall completed."
    } else {
        Write-Host "Microsoft MPI not found. Installing..."
    }

    # Pick the exact installer already downloaded by vcpkg (best option)
    $candidate = Get-ChildItem -Path $VcpkgDownloadsDir -Filter "msmpisetup-$ExpectedVersion*.exe" -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1

    if (-not $candidate) {
        throw "Expected MS-MPI installer not found in $VcpkgDownloadsDir. Run vcpkg once (so it downloads it), or place it there."
    }

    Write-Host "Installing Microsoft MPI from: $($candidate.FullName)"
    Start-Process -FilePath $candidate.FullName -ArgumentList "-unattend","-force" -Wait
    Write-Host "Microsoft MPI installed successfully (expected $ExpectedVersion)."
}

# Detect operating system and execute the appropriate installation function
$OS = [System.Environment]::OSVersion.Platform
if ($OS -eq "Win32NT")
{
    Set-Location "C:\"

    Write-Host "Starting the installation process on Windows..."

    # Attempt to locate an existing Visual Studio installation (must have C++ tools).
    # If not found (or vswhere not available), download & install VS Community, then re-check.

    # Try to find vswhere (either in PATH or in the default installer folder)
    $vswhereCmd = Get-Command vswhere -ErrorAction SilentlyContinue
    if (-not $vswhereCmd) {
        $defaultVswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $defaultVswhere) {
            $vswhereCmd = $defaultVswhere
        }
    }

    # If we have vswhere, query for an installation with C++ tools
    $vsInstallPath = $null
    if ($vswhereCmd) {
        $vsInstallPath = & $vswhereCmd -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    }

    # If not found, install VS Community with your existing bootstrapper flow
    if (-not $vsInstallPath -or -not (Test-Path "$vsInstallPath\Common7\Tools\VsDevCmd.bat")) {
        Write-Host "No suitable Visual Studio installation found. Installing Community Edition with C++ tools..."

        $VISUAL_STUDIO_INSTALLER = "C:\VisualStudioSetup.exe"
        Invoke-WebRequest -Uri "https://c2rsetup.officeapps.live.com/c2r/downloadVS.aspx?sku=community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030:108d217f1e244b9aa0326ce9a131978a" -OutFile $VISUAL_STUDIO_INSTALLER
        Start-Process -FilePath $VISUAL_STUDIO_INSTALLER -Wait
        Remove-Item $VISUAL_STUDIO_INSTALLER

        # After install, try again to find vswhere and query the installation path
        $vswhereCmd = Get-Command vswhere -ErrorAction SilentlyContinue
        if (-not $vswhereCmd) {
            $defaultVswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
            if (Test-Path $defaultVswhere) {
                $vswhereCmd = $defaultVswhere
            }
        }

        if ($vswhereCmd) {
            $vsInstallPath = & $vswhereCmd -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        }
    }

    # Final check and activation of the developer environment
    if ($vsInstallPath -and (Test-Path "$vsInstallPath\Common7\Tools\VsDevCmd.bat")) {
        Write-Host "Using Visual Studio installation at $vsInstallPath"
        & "$vsInstallPath\Common7\Tools\VsDevCmd.bat"
    } else {
        Write-Error "Failed to detect Visual Studio with required C++ components after installation."
        exit 1
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

    # Detect cmake-gui availability
    $cmakeGuiCmd = Get-Command cmake-gui -ErrorAction SilentlyContinue
    if (-not $cmakeGuiCmd) {
        $cmakeGuiCmd = Get-Command "cmake-gui.exe" -ErrorAction SilentlyContinue
    }
    $HAS_CMAKE_GUI = ($null -ne $cmakeGuiCmd) -and (-not ($env:CI -eq "true"))

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
    $OverlayRoot = Join-Path $env:VCPKG_ROOT 'overlays\ports'
    if (-not (Test-Path $OverlayRoot)) {
        New-Item -ItemType Directory -Force -Path $OverlayRoot | Out-Null
    }

    # Install CPLEX
    if (-not $withoutCplex) {
        Write-Host "Installing CPLEX..."
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
        Write-Host "... CPLEX installed succesfully."
    }

    # Install Gurobi
    if (-not $withoutGurobi) {
        Write-Host "Installing Gurobi..."
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
        Write-Host "... Gurobi installed succesfully."
    }

    # Install SCIP
    if (-not $withoutSCIP) {
        Write-Host "Installing SCIP..."
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
        Write-Host "... SCIP installed succesfully."
    }

    # Install StOpt
    if (-not $withoutStOpt) {
        Write-Host "Installing StOpt..."
        $StOpt_ROOT = "C:\StOpt"
        if (-not (Test-Path $StOpt_ROOT)) {
            Write-Host "" # new line
            # Configure vcpkg
            git clone https://gitlab.com/stochastic-control/StOpt.git $StOpt_ROOT
            Set-Location $StOpt_ROOT

            # Install / Upgrade Microsoft MPI
            $expectedMsmpiVersion = "10.1.12498.52"
            Write-Host "Checking Microsoft MPI installation..."
            $downloadsDir = Join-Path $env:VCPKG_ROOT 'downloads'
            if (-not (Test-Path $downloadsDir)) {
                New-Item -ItemType Directory -Force -Path $downloadsDir | Out-Null
            }
            # Optional: ensure the correct installer exists (use vcpkg downloads if present, otherwise download it)
            $msmpiInstaller = Get-ChildItem -Path $downloadsDir -Filter "msmpisetup-$expectedMsmpiVersion*.exe" -ErrorAction SilentlyContinue |
                    Sort-Object LastWriteTime -Descending | Select-Object -First 1
            if (-not $msmpiInstaller) {
                Write-Host "Expected MS-MPI installer not found in vcpkg downloads. Downloading..."
                $msmpiInstallerPath = Join-Path $downloadsDir "msmpisetup-$expectedMsmpiVersion.exe"
                # This is the same CDN url vcpkg used in your log for .52 (runtime setup)
                $msmpiUrl = "https://download.microsoft.com/download/7/2/7/72731ebb-b63c-4170-ade7-836966263a8f/msmpisetup.exe"
                Invoke-WebRequest -Uri $msmpiUrl -OutFile $msmpiInstallerPath
            }
            else {
                $msmpiInstallerPath = $msmpiInstaller.FullName
            }
            # This enforces the expected version:
            # - If not installed -> install
            # - If installed but different version -> uninstall old + install new
            Write-Host "Ensuring Microsoft MPI version $expectedMsmpiVersion..."
            Ensure-MsMpiVersion -ExpectedVersion $expectedMsmpiVersion -VcpkgDownloadsDir $downloadsDir
            Write-Host "... Microsoft MPI is aligned with expected version $expectedMsmpiVersion."

            mv .\doc "C:\" # TODO remove when the doc bug in StOpt will be fixed
            # Configure once using multi-config
            $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
            & cmake -S . -B 'build' -G "Visual Studio 17 2022" `
                    '-DBUILD_PYTHON=OFF' `
                    '-DBUILD_TEST=OFF' `
                    "-DCMAKE_INSTALL_PREFIX=$StOpt_ROOT" `
                    '-Wno-dev'
            # Build Debug
            & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
            & cmake '--install' 'build' '--config' 'Debug'
            # Build Release
            & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
            & cmake '--install' 'build' '--config' 'Release'
            mv "C:\doc" $StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
            Write-Host "... StOpt installed succesfully."
        } else {
            Set-Location $StOpt_ROOT
            git remote update
            $local = git rev-parse "@"
            $remote = git rev-parse "@{u}"
            if ($local -ne $remote) { # StOpt is not latest
                git pull
                Write-Host "" # new line
                mv .\doc "C:\" # TODO remove when the doc bug in StOpt will be fixed
                # Re-configure once using multi-config
                $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
                & cmake -S . -B 'build' -G "Visual Studio 17 2022" `
                        '-DBUILD_PYTHON=OFF' `
                        '-DBUILD_TEST=OFF' `
                        "-DCMAKE_INSTALL_PREFIX=$StOpt_ROOT" `
                        '-Wno-dev'
                # Rebuild Debug
                & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
                & cmake '--install' 'build' '--config' 'Debug'
                # Rebuild Release
                & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
                & cmake '--install' 'build' '--config' 'Release'
                mv "C:\doc" $StOpt_ROOT # TODO remove when the doc bug in StOpt will be fixed
                Write-Host "... StOpt updated succesfully."
            } else {
                Write-Host "... StOpt already up to date."
            }
            Set-Location "C:\"
        }
        # Add StOpt to the system PATH
        Add-ToSystemPath -PathToAdd "$StOpt_ROOT\bin"
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
        git pull --recurse-submodules
    } else {
        Write-Host "Repository not found locally. Cloning SMSpp..."
        if (-not $HAS_CMAKE_GUI) {
            # no way to use cmake-gui interactively to choose submodules, so download all
            git clone --branch develop --recurse-submodules https://gitlab.com/smspp/smspp-project.git $SMSPP_ROOT
        } else {
            git clone --branch develop https://gitlab.com/smspp/smspp-project.git $SMSPP_ROOT
        }
        Set-Location $SMSPP_ROOT
    }

    # Update builtin-baseline in vcpkg.json to match the current vcpkg commit
    if ($updatevcpkg) {
        $ManifestPath = Join-Path $SMSPP_ROOT 'vcpkg.json'
        $Baseline = (& git -C $env:VCPKG_ROOT rev-parse HEAD).Trim()
        $manifestJson = Get-Content $ManifestPath -Raw | ConvertFrom-Json
        $manifestJson.'builtin-baseline' = $Baseline
        $manifestJson | ConvertTo-Json -Depth 10 | Set-Content $ManifestPath -Encoding UTF8
    }

    # Configure COIN-OR Osi
    Write-Host "Configuring COIN-OR Osi..."

    # Prepare an overlay port so vcpkg uses OUR modified portfile.cmake
    if ((-not $withoutCplex) -or (-not $withoutGurobi)) {
        $OverlayPort = Join-Path $OverlayRoot 'coin-or-osi'
        New-Item -ItemType Directory -Force -Path $OverlayPort | Out-Null

        # Copy the original port as a base for our overlay
        Copy-Item -Recurse -Force (Join-Path $env:VCPKG_ROOT 'ports\coin-or-osi\*') $OverlayPort

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
        $env:VCPKG_OVERLAY_PORTS = $OverlayRoot
    } else {
        Remove-Item Env:\VCPKG_OVERLAY_PORTS -ErrorAction SilentlyContinue
        $OverlayPort = Join-Path $OverlayRoot 'coin-or-osi'
        if (Test-Path $OverlayPort) { Remove-Item -Recurse -Force $OverlayPort }
    }

    Set-Location $SMSPP_ROOT

    # Configure once using multi-config
    $env:VCPKG_BINARY_SOURCES = 'clear;default' # avoid stale cached binaries
    $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    & cmake -S . -B 'build' -G "Visual Studio 17 2022" "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT" '-Wno-dev'
    # run cmake-gui
    if ($HAS_CMAKE_GUI) {
        # select submodules, then Configure and Generate the build files
        Start-Process -FilePath $cmakeGuiCmd.Source -ArgumentList 'build' -Wait
    }

    <## Build Debug
    & cmake '--build' 'build' '--config' 'Debug' "-j $MAX_JOBS"
    & cmake '--install' 'build' '--config' 'Debug'#>

    # Build Release
    & cmake '--build' 'build' '--config' 'Release' "-j $MAX_JOBS"
    & cmake '--install' 'build' '--config' 'Release'

    # Add SMSpp to the system PATH
    $SMSPP_BIN = "$SMSPP_ROOT\bin"
    Add-ToSystemPath -PathToAdd $SMSPP_BIN

    $VCPKG_BIN = Join-Path $SMSPP_ROOT "build\vcpkg_installed\x64-windows\bin"
    Add-ToSystemPath -PathToAdd $VCPKG_BIN

    refreshenv
}
