<#
    .SYNOPSIS
    This script installs SMS++ and all its dependencies.

    .DESCRIPTION
    This script performs the installation of SMS++ and all its dependencies.
    If not already present, it clones the smspp-project repositories, then builds and installs them.

    You can use the `-installRoot <your-custom-path>` option to specify your SMS++ custom installation root.
    You can use the `-cplexInstaller <path-to-installer>` option to point to your own CPLEX installer.
    You can use the `-updatevcpkg` option to update the builtin-baseline in vcpkg.json.
    You can use the `-withoutCplex` option to skip the installation of CPLEX.
    You can use the `-withoutGurobi` option to skip the installation of Gurobi.
    You can use the `-withoutMosek` option to skip the installation of MOSEK.
    You can use the `-withoutSCIP` option to skip the installation of SCIP.
    You can use the `-withoutClarabel` option to skip the installation of Clarabel, whose C
    interface is built with the Rust toolchain (cargo).
    Note that PIPS-IPM++ (PIPSMILPSolver) is not supported on Windows, so there is no related option.
    Note that HPIPM (HPIPMMILPSolver), which is in no vcpkg registry, is not installed on Windows,
    so there is no related option.
    Note that PERMON (PERMONMILPSolver), which is in no vcpkg registry, is not installed on Windows,
    so there is no related option.
    Note that HiGHS, DAQP, qpOASES, ProxSuite, PIQP, OSQP, qpmad, quadprog, StOpt, abLEMON (the
    port ablemon), LIBSVM and LIBLINEAR come from vcpkg (vcpkg.json), so there are no related options:
    DAQP, PIQP, qpmad, quadprog and ablemon, which the default registry of vcpkg does not carry,
    come from the SMS++ vcpkg registry (https://gitlab.com/smspp/vcpkg-registry). quadprog is
    under the GPL, so that a MILPSolver built with it is too.
    Note that MiniSat comes from vcpkg (vcpkg.json, the port minisat-master-keying), while
    CaDiCaL is not packaged for Windows, so there are no related options: SATBlock is built
    with MiniSATSolver and without CaDiCaLSATSolver.
    Note that RECORD and COMBO use POSIX clocks and 128-bit integers, which MSVC does not have,
    so there are no related options: BinaryKnapsackBlock is built without the Solver that wrap them.
    You can use the `-withoutTorch` option to skip the installation of Torch.
    You can use the `-withoutSMSpp` option to skip the installation of SMS++.
    Run again, the script brings up to date what it installed: Clarabel goes to its latest
    release and is built again only when it changed, Gurobi, MOSEK, SCIP and Torch are installed
    again when the version asked for changes (the old installation is set aside, and put back if
    the new one fails), and vcpkg and SMS++ are pulled; CPLEX, whose installer needs a license,
    is never touched once installed.
    You can use the `-withExperiments` option to also fetch and build the private experiments
    submodule, which needs access to its repository; without it the script never touches it.
    You can use the `-withoutDefenderExclusions` option to skip adding the Windows Defender path
    exclusions for the vcpkg and SMS++ directories (added by default to avoid spurious
    "Permission denied" / "File exists" errors when an antivirus locks or quarantines freshly
    built executables; if you run a third-party antivirus you must exclude those paths in it too).

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

        .\INSTALL.ps1 -installRoot <your-custom-path> -without<some-dependency>

    If you have not yet cloned the SMS++ repository, you can run the script directly:

        & ([scriptblock]::Create((New-Object System.Net.WebClient).DownloadString('https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.ps1'))) -installRoot <your-custom-path> -without<some-dependency>

#>

# Default values indicating if dependencies should be installed
param(
    [switch]$withoutCplex,
    [switch]$withoutGurobi,
    [switch]$withoutMosek,
    [switch]$withoutSCIP,
    [switch]$withoutClarabel,
    [switch]$withoutTorch,
    [switch]$withoutSMSpp,
    [switch]$withExperiments,
    [switch]$updatevcpkg,
    [switch]$withoutDefenderExclusions,
    [string]$installRoot = "C:\", # Default if not provided
    [string]$cplexInstaller = "" # Path to a user-provided CPLEX installer
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

function Add-DefenderExclusions
{
    param(
        [Parameter(Mandatory=$true)][string[]]$Paths
    )

    # Antivirus real-time scanning can lock or quarantine freshly built, unsigned executables
    # under the build trees (e.g. OpenBLAS getarch_2nd.exe, bkbench.exe), making CMake/vcpkg
    # fail to copy them ("Permission denied" / "File exists"). We exclude the build directories
    # from Windows Defender. CLI only, so it works on CI. Add-MpPreference only drives Defender:
    # it fails (0x800106ba) when a third-party antivirus (Avast, etc.) is the active provider,
    # in which case the user must add the same exclusions in their own antivirus, or disable it.
    if (Get-Command Add-MpPreference -ErrorAction SilentlyContinue) {
        foreach ($p in $Paths) {
            try {
                Add-MpPreference -ExclusionPath $p -ErrorAction Stop
                Write-Host "Added Windows Defender exclusion: $p"
            } catch {
                Write-Warning "Could not add Defender exclusion for ${p}: $($_.Exception.Message)"
            }
        }
    } else {
        Write-Host "Add-MpPreference not available; skipping Windows Defender exclusions."
    }

    # If a third-party antivirus is active, Defender exclusions do not apply to it: warn loudly.
    try {
        $thirdPartyAv = Get-CimInstance -Namespace 'root/SecurityCenter2' -ClassName AntiVirusProduct -ErrorAction Stop |
                Where-Object { $_.displayName -and $_.displayName -notmatch 'Windows Defender' }
        if ($thirdPartyAv) {
            $names = ($thirdPartyAv.displayName | Sort-Object -Unique) -join ', '
            Write-Warning "Third-party antivirus detected ($names); Windows Defender exclusions do not cover it."
            Write-Warning "Exclude these paths in your antivirus, or disable it during the build, to avoid"
            Write-Warning "'Permission denied' / 'File exists' errors on freshly built executables:"
            foreach ($p in $Paths) { Write-Warning "    $p" }
        }
    } catch {
        # root/SecurityCenter2 is unavailable on Windows Server / CI; nothing to warn about there.
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

# Clone the repository $Url into the directory $Dir, or bring an existing
# clone up to date, and say whether there is something new to build: with a
# pattern (a regular expression) the clone goes to the latest tag that matches
# it, e.g., the latest release, otherwise it follows its branch, pulling what
# is new
function Update-GitLatest
{
    param(
        [Parameter(Mandatory=$true)][string]$Url,
        [Parameter(Mandatory=$true)][string]$Dir,
        [string]$Pattern = ""
    )

    $fresh = $false
    if (-not (Test-Path (Join-Path $Dir ".git"))) {
        git clone -q $Url $Dir | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "Update-GitLatest: cannot clone $Url" }
        $fresh = $true
    }
    $before = (git -C $Dir rev-parse HEAD).Trim()
    if ($Pattern) {
        git -C $Dir fetch -q --tags --force origin | Out-Null
        $tag = git -C $Dir tag --sort=-v:refname |
                Where-Object { $_ -match $Pattern } | Select-Object -First 1
        if ($tag) { git -C $Dir checkout -q $tag | Out-Null }
    } elseif (-not $fresh) {
        git -C $Dir pull -q --ff-only | Out-Null
    }
    git -C $Dir submodule update -q --init --recursive | Out-Null
    $after = (git -C $Dir rev-parse HEAD).Trim()
    return ($fresh -or ($before -ne $after))
}

# Whether the dependency installed in $Root is not at the version $Version
# (or is not there at all): an installation older than this check has no
# .smspp-version, and it is taken as being at the version asked for
function Test-VersionDiffers
{
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$Version
    )

    if (-not (Test-Path $Root)) { return $true }
    $versionFile = Join-Path $Root ".smspp-version"
    if (-not (Test-Path $versionFile)) {
        Set-Content -Path $versionFile -Value $Version
        return $false
    }
    return ((Get-Content $versionFile -Raw).Trim() -ne $Version)
}

# Before a dependency is installed again, its installation is moved aside:
# it is removed once the new one is in place [see Set-InstalledVersion], and
# put back if the installation fails [see Restore-SetAside]
function Set-Aside
{
    param([Parameter(Mandatory=$true)][string]$Root)

    if (Test-Path $Root) {
        Remove-Item "$Root.old" -Recurse -Force -ErrorAction SilentlyContinue
        Move-Item -Path $Root -Destination "$Root.old"
    }
}

function Restore-SetAside
{
    param([Parameter(Mandatory=$true)][string]$Root)

    if (Test-Path "$Root.old") {
        Remove-Item $Root -Recurse -Force -ErrorAction SilentlyContinue
        Move-Item -Path "$Root.old" -Destination $Root
    }
}

# The version $Version of the dependency just installed in $Root, and the
# installation it replaced removed
function Set-InstalledVersion
{
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$Version
    )

    if (Test-Path $Root) {
        Set-Content -Path (Join-Path $Root ".smspp-version") -Value $Version
    }
    Remove-Item "$Root.old" -Recurse -Force -ErrorAction SilentlyContinue
}

# Detect operating system and execute the appropriate installation function
$OS = [System.Environment]::OSVersion.Platform
if ($OS -eq "Win32NT")
{
    Set-Location "C:\"

    Write-Host "Starting the installation process on Windows..."

    # Add Windows Defender path exclusions for the build directories before any build runs
    if (-not $withoutDefenderExclusions) {
        Add-DefenderExclusions -Paths @($env:VCPKG_ROOT, "$installRoot\smspp-project")
    }

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
    }
    $env:VCPKG_FEATURE_FLAGS = 'manifests,registries'

    # Install CPLEX
    if (-not $withoutCplex) {
        Write-Host "Installing CPLEX..."
        $CPLEX_ROOT = "C:\IBM\ILOG\CPLEX_Studio"
        if (-not (Test-Path $CPLEX_ROOT)) {
            Set-Location "C:\"
            if (-not $cplexInstaller -or -not (Test-Path $cplexInstaller)) {
                Write-Host "CPLEX installer not provided. Download IBM ILOG CPLEX Optimization"
                Write-Host "Studio yourself (e.g. through the IBM Academic Initiative) and either"
                Write-Host "install it manually or re-run with -cplexInstaller <path-to-installer>."
                Write-Host "Skipping CPLEX."
                $withoutCplex = $true
            } else {
                Start-Process -FilePath $cplexInstaller -Wait
                # Move "IBM" folder from "C:\Program Files" to "C:\", so that its path
                # has no spaces
                Move-Item -Path "C:\Program Files\IBM" -Destination "C:\IBM"
                # the installer creates ILOG\CPLEX_Studio<ver>; strip the version from the path
                $cplexVersioned = Get-ChildItem -Path "C:\IBM\ILOG" -Directory -Filter "CPLEX_Studio*" | Select-Object -First 1
                Move-Item -Path $cplexVersioned.FullName -Destination $CPLEX_ROOT -ErrorAction SilentlyContinue
                # Update the system PATH to ensure the SMS++ exe can correctly locate the cplex*.dll file
                Update-EnvironmentVariables -oldPattern "C:\Program Files\IBM\ILOG\$($cplexVersioned.Name)" -newValue $CPLEX_ROOT
                Write-Host "... CPLEX installed successfully."
            }
        } else {
            Write-Host "... CPLEX installed successfully."
        }
    }

    # Install Gurobi
    if (-not $withoutGurobi) {
        Write-Host "Installing or updating Gurobi..."
        $GUROBI_ROOT = "C:\gurobi"
        $GUROBI_VERSION = "12.0.1"
        if (Test-VersionDiffers -Root $GUROBI_ROOT -Version $GUROBI_VERSION) {
            Set-Aside -Root $GUROBI_ROOT
            try {
                Set-Location "C:\"
                $GUROBI_MAJOR_MINOR = $GUROBI_VERSION.Substring(0, $GUROBI_VERSION.LastIndexOf('.'))
                $GUROBI_DIR = "gurobi" + $GUROBI_VERSION.Replace('.', '')
                $GUROBI_INSTALLER = "Gurobi-$GUROBI_VERSION-win64.msi"
                Invoke-WebRequest -Uri "https://packages.gurobi.com/$GUROBI_MAJOR_MINOR/$GUROBI_INSTALLER" -OutFile "C:\$GUROBI_INSTALLER"
                Start-Process -FilePath "msiexec.exe" -ArgumentList "/i", "C:\$GUROBI_INSTALLER" -Wait
                Remove-Item "C:\$GUROBI_INSTALLER"
                Move-Item -Path "C:\$GUROBI_DIR" -Destination $GUROBI_ROOT -ErrorAction Stop
                # Update the system PATH to ensure the SMS++ exe can correctly locate the gurobi*.dll file
                Update-EnvironmentVariables -oldPattern "C:\$GUROBI_DIR" -newValue $GUROBI_ROOT
                Set-InstalledVersion -Root $GUROBI_ROOT -Version $GUROBI_VERSION
            } catch {
                Restore-SetAside -Root $GUROBI_ROOT
                throw
            }
            Write-Host "... Gurobi installed successfully."
        } else {
            Write-Host "... Gurobi already at its version."
        }
    }

    # Install MOSEK: the installer of the latest 12.0.x version, whose
    # number is published next to it (a fixed one if it cannot be read); it
    # installs into C:\Program Files\Mosek\<major.minor>, and MOSEK_ROOT is
    # C:\Program Files\Mosek, which holds one directory per version
    if (-not $withoutMosek) {
        Write-Host "Installing or updating MOSEK..."
        $MOSEK_ROOT = "C:\Program Files\Mosek"
        $MOSEK_URL = "https://download.mosek.com/stable"
        try {
            $MOSEK_VERSION = (Invoke-WebRequest -UseBasicParsing `
                -Uri "$MOSEK_URL/12.0/version").Content.Trim()
        } catch {
            $MOSEK_VERSION = ""
        }
        if ($MOSEK_VERSION -notmatch '^12\.0\.[0-9]+$') {
            $MOSEK_VERSION = "12.0.0"
        }
        # the installer of a newer version upgrades the one installed, in
        # the same directory of its major.minor version
        if (Test-VersionDiffers -Root $MOSEK_ROOT -Version $MOSEK_VERSION) {
            Set-Location "C:\"
            $MOSEK_INSTALLER = "moseksetupwin64x86.msi"
            Invoke-WebRequest `
                -Uri "$MOSEK_URL/$MOSEK_VERSION/$MOSEK_INSTALLER" `
                -OutFile "C:\$MOSEK_INSTALLER"
            Start-Process -FilePath "msiexec.exe" `
                -ArgumentList "/i", "C:\$MOSEK_INSTALLER" -Wait
            Remove-Item "C:\$MOSEK_INSTALLER"
            # Update the system PATH to ensure the SMS++ exe can correctly
            # locate the mosek*.dll file
            $MOSEK_MAJOR_MINOR = $MOSEK_VERSION.Substring(0,
                $MOSEK_VERSION.LastIndexOf('.'))
            Add-ToSystemPath -PathToAdd `
                "$MOSEK_ROOT\$MOSEK_MAJOR_MINOR\tools\platform\win64x86\bin"
            Set-InstalledVersion -Root $MOSEK_ROOT -Version $MOSEK_VERSION
            Write-Host "... MOSEK installed successfully."
        } else {
            Write-Host "... MOSEK already at its version."
        }
        Write-Host "MOSEK needs a license (free for academic use, see"
        Write-Host "https://www.mosek.com/products/academic-licenses/):"
        Write-Host "put it in %USERPROFILE%\mosek\mosek.lic or point"
        Write-Host "MOSEKLM_LICENSE_FILE to it."
    }

    # Install SCIP
    if (-not $withoutSCIP) {
        Write-Host "Installing or updating SCIP..."
        $SCIP_ROOT = "C:\Program Files\SCIPOptSuite"
        $SCIP_VERSION = "9.0.0"
        if (Test-VersionDiffers -Root $SCIP_ROOT -Version $SCIP_VERSION) {
            Set-Aside -Root $SCIP_ROOT
            try {
                Set-Location "C:\"
                $SCIP_INSTALLER = "SCIPOptSuite-$SCIP_VERSION-win64-VS15.exe"
                Invoke-WebRequest -Uri "https://www.scipopt.org/download/release/$SCIP_INSTALLER" -OutFile "C:\$SCIP_INSTALLER"
                Start-Process -FilePath "C:\$SCIP_INSTALLER" -Wait
                Remove-Item "C:\$SCIP_INSTALLER"
                Move-Item -Path "C:\Program Files\SCIPOptSuite $SCIP_VERSION" -Destination $SCIP_ROOT -ErrorAction Stop
                # Update the system PATH to ensure the SMS++ exe can correctly locate the scip*.dll file
                Update-EnvironmentVariables -oldPattern "C:\Program Files\SCIPOptSuite $SCIP_VERSION" -newValue $SCIP_ROOT
                Set-InstalledVersion -Root $SCIP_ROOT -Version $SCIP_VERSION
            } catch {
                Restore-SetAside -Root $SCIP_ROOT
                throw
            }
            Write-Host "... SCIP installed successfully."
        } else {
            Write-Host "... SCIP already at its version."
        }
    }

    # Install Torch
    if (-not $withoutTorch) {
        Write-Host "Installing or updating Torch..."
        $Torch_ROOT = "C:\torch"
        $TORCH_VERSION = "2.12.0"
        if (Test-VersionDiffers -Root $Torch_ROOT -Version $TORCH_VERSION) {
            Set-Aside -Root $Torch_ROOT
            try {
                Set-Location "C:\"
                $TORCH_INSTALLER = "libtorch-win-shared-with-deps-$TORCH_VERSION%2Bcpu.zip"
                Invoke-WebRequest -Uri "https://download.pytorch.org/libtorch/cpu/$TORCH_INSTALLER" -OutFile "C:\torch.zip"
                Expand-Archive -Path "C:\torch.zip" -DestinationPath "C:\" -Force
                Remove-Item "C:\torch.zip"
                # The archive unpacks as libtorch, rename to the version-less torch
                Move-Item -Path "C:\libtorch" -Destination $Torch_ROOT -ErrorAction Stop
                # Update the system PATH to ensure the SMS++ exe can correctly locate the torch*.dll file
                Add-ToSystemPath -PathToAdd "$Torch_ROOT\lib"
                Set-InstalledVersion -Root $Torch_ROOT -Version $TORCH_VERSION
            } catch {
                Restore-SetAside -Root $Torch_ROOT
                throw
            }
            Write-Host "... Torch installed successfully."
        } else {
            Write-Host "... Torch already at its version."
        }
    }

    # Install Clarabel: the C interface of Clarabel (Clarabel.cpp) at its
    # latest release, whose DLL is built by cargo from the Rust solver it
    # carries as a submodule; its CMake installs nothing, hence the headers
    # are taken from its include directory and the DLL and its import
    # library are copied into its lib directory, where SMS++ looks for them
    # [see extlib/makefile-default-paths-win]
    if (-not $withoutClarabel) {
        Write-Host "Installing or updating Clarabel..."
        $Clarabel_ROOT = "C:\clarabel"
        if (-not (Get-Command cargo -ErrorAction SilentlyContinue)) {
            choco install rust-ms -y --limit-output
            Import-Module $env:ChocolateyInstall\helpers\chocolateyProfile.psm1
            refreshenv
        }
        $clarabelFresh = -not (Test-Path (Join-Path $Clarabel_ROOT ".git"))
        try {
            if (Update-GitLatest -Url "https://github.com/oxfordcontrol/Clarabel.cpp.git" `
                    -Dir $Clarabel_ROOT -Pattern '^v[0-9]+\.[0-9]+\.[0-9]+$') {
                Set-Location "$Clarabel_ROOT\rust_wrapper"
                cargo build --release
                if ($LASTEXITCODE -ne 0) { throw "cargo could not build Clarabel" }
                New-Item -ItemType Directory -Force -Path "$Clarabel_ROOT\lib" | Out-Null
                Copy-Item -Force "target\release\clarabel_c.dll", "target\release\clarabel_c.dll.lib" `
                    -Destination "$Clarabel_ROOT\lib"
                # Update the system PATH to ensure the SMS++ exe can correctly locate clarabel_c.dll
                Add-ToSystemPath -PathToAdd "$Clarabel_ROOT\lib"
                Write-Host "... Clarabel installed successfully."
            } else {
                Write-Host "... Clarabel already up to date."
            }
        } catch {
            # only a new clone is removed: a failed update leaves the
            # installation that was there
            if ($clarabelFresh) {
                Set-Location "C:\"
                Remove-Item $Clarabel_ROOT -Recurse -Force -ErrorAction SilentlyContinue
            }
            throw
        }
        Set-Location "C:\"
    }

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

    # experiments is private: unless asked for, it is neither fetched nor updated, whatever
    # the local configuration says, so that git never stops to ask for credentials
    $skipExperiments = @()
    if (-not $withExperiments) {
        $skipExperiments = @('-c', 'submodule.experiments.update=none',
                             '-c', 'submodule.experiments.fetchRecurseSubmodules=false')
    }

    # Check if the SMSpp repository already exists
    if (Test-Path $SMSPP_ROOT) {
        Set-Location $SMSPP_ROOT
        Write-Host "SMSpp already exists. Pulling latest changes..."
        git @skipExperiments pull
        git submodule sync --recursive
        git @skipExperiments submodule update --recursive
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
    if ($withExperiments) {
        git config submodule.experiments.update checkout
        git submodule update --init experiments
    }
    $experimentsFlag = if ($withExperiments) { '-DBUILD_experiments=ON' } else { '-DBUILD_experiments=OFF' }

    # Update builtin-baseline in vcpkg.json to match the current vcpkg commit
    if ($updatevcpkg) {
        $ManifestPath = Join-Path $SMSPP_ROOT 'vcpkg.json'
        $Baseline = (& git -C $env:VCPKG_ROOT rev-parse HEAD).Trim()
        $manifestJson = Get-Content $ManifestPath -Raw | ConvertFrom-Json
        $manifestJson.'builtin-baseline' = $Baseline
        $manifestJson | ConvertTo-Json -Depth 10 | Set-Content $ManifestPath -Encoding UTF8
    }

    Set-Location $SMSPP_ROOT

    # Configure once using multi-config
    $env:VCPKG_BINARY_SOURCES = 'clear;default' # avoid stale cached binaries
    $env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    $env:VCPKG_TARGET_TRIPLET="x64-windows"
    & cmake -S . -B 'build' -G "Visual Studio 17 2022" "-DCMAKE_INSTALL_PREFIX=$SMSPP_ROOT" '-Wno-dev' $experimentsFlag
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

    $VCPKG_BIN = Join-Path $SMSPP_ROOT "build\vcpkg_installed\$env:VCPKG_TARGET_TRIPLET\bin"
    Add-ToSystemPath -PathToAdd $VCPKG_BIN

    refreshenv
}
