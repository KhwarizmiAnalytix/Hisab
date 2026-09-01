# XSigma CI - Windows Dependency Installation Script (PowerShell)
# This script installs all required dependencies for building XSigma on Windows
# Usage: .\install-deps-windows.ps1 [-WithCuda] [-WithTbb]

param(
    [switch]$WithCuda = $false,
    [switch]$WithTbb = $false
)

# Color codes for output
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Error handling
$ErrorActionPreference = "Stop"

Write-Info "Starting Windows dependency installation..."
Write-Info "CUDA support: $WithCuda"
Write-Info "TBB support: $WithTbb"

$sanitizeScript = Join-Path $PSScriptRoot "sanitize_thirdparty_cache.py"
if ((Test-Path $sanitizeScript) -and (Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Info "Sanitizing cached ThirdParty CMake trees (workspace path check)..."
    try {
        python $sanitizeScript
    } catch {
        Write-Warning "ThirdParty cache sanitize skipped: $_"
    }
}

# Check if Chocolatey is installed
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Error "Chocolatey is not installed. Please install Chocolatey first:"
    Write-Info "  Run PowerShell as Administrator and execute:"
    Write-Info "  Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
    exit 1
}

Write-Info "Chocolatey found. Installing dependencies..."

# Core build tools
Write-Info "Installing core build tools..."
try {
    choco install -y cmake ninja git curl wget python || {
        Write-Error "Failed to install core build tools"
        exit 1
    }
} catch {
    Write-Error "Failed to install core build tools: $_"
    exit 1
}

# Clang compiler
Write-Info "Installing Clang compiler..."
try {
    choco install -y llvm || {
        Write-Warning "Failed to install LLVM/Clang"
    }
} catch {
    Write-Warning "Failed to install LLVM/Clang: $_"
}

# buildcache compiler cache
Write-Info "Installing buildcache compiler cache..."
$buildcacheVersion = $env:BUILDCACHE_VERSION
if ([string]::IsNullOrWhiteSpace($buildcacheVersion)) {
    $buildcacheVersion = "0.28.4"
}
try {
    $buildcacheRoot = Join-Path $env:USERPROFILE ".local\bin"
    New-Item -ItemType Directory -Force -Path $buildcacheRoot | Out-Null
    $zipPath = Join-Path $env:TEMP "buildcache.zip"
    $buildcacheUrl = "https://github.com/mbitsnbites/buildcache/releases/download/v$buildcacheVersion/buildcache-windows.zip"
    Write-Info "Downloading buildcache from $buildcacheUrl"
    Invoke-WebRequest -Uri $buildcacheUrl -OutFile $zipPath -UseBasicParsing
    Expand-Archive -Path $zipPath -DestinationPath $buildcacheRoot -Force
    Remove-Item $zipPath -Force
    $exe = Get-ChildItem -Path $buildcacheRoot -Filter buildcache.exe -Recurse | Select-Object -First 1
    if (-not $exe) {
        throw "buildcache.exe not found after extraction"
    }
    $targetExe = Join-Path $buildcacheRoot "buildcache.exe"
    Copy-Item $exe.FullName -Destination $targetExe -Force
    if ($exe.DirectoryName -ne $buildcacheRoot) {
        Remove-Item $exe.DirectoryName -Recurse -Force
    }
    if ($env:GITHUB_PATH) {
        Add-Content -Path $env:GITHUB_PATH -Value $buildcacheRoot
    } else {
        $env:Path = "$buildcacheRoot;$env:Path"
    }
    $buildcacheVersionOutput = & $targetExe --version
    Write-Success "buildcache installed: $buildcacheVersionOutput"
} catch {
    Write-Warning "Failed to install buildcache: $_"
}

# Visual Studio (MSVC). GitHub windows-latest ships VS 2022 Enterprise, not
# BuildTools. Only the BuildTools path was checked before, so CI tried a
# Chocolatey install that fails ("no 'version=' field") and left CMake unable
# to select the Visual Studio 17 2022 generator.
function Get-Vs2022InstallPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        return $null
    }
    $path = & $vswhere -latest -products * -version "[17.0,18.0)" `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath 2>$null
    if ($path) {
        return ([string]$path).Trim()
    }
    return $null
}

Write-Info "Checking for Visual Studio 2022 with C++ tools..."
$vsInstall = Get-Vs2022InstallPath
if ($vsInstall) {
    Write-Success "Visual Studio 2022 found: $vsInstall"
} else {
    Write-Warning "No VS 2022 C++ toolchain found via vswhere; not installing via Chocolatey (that package fails on hosted runners)."
}

# TBB (Threading Building Blocks)
# Chocolatey's `tbb` package still points at TBB 4.1 (2013) on
# threadingbuildingblocks.org, which now 403s. oneTBB ships official
# MSVC-ABI Windows binaries; Clang/lld can link those, and CMake refuses
# to build TBB from source on Windows+Clang (see tbb_memory.cmake).
if ($WithTbb) {
    Write-Info "Installing Intel oneTBB (official Windows binaries)..."
    $onetbbVersion = "2023.1.0"
    $onetbbSha256 = "cf6ee0c600fcb5c3a9b65e3e6e4781669d06f1bb1e37970d145fcde08eed8da9"
    $onetbbUrl = "https://github.com/uxlfoundation/oneTBB/releases/download/v$onetbbVersion/oneapi-tbb-$onetbbVersion-win.zip"
    $installRoot = Join-Path $env:USERPROFILE "oneTBB"
    $tbbRoot = Join-Path $installRoot "oneapi-tbb-$onetbbVersion"
    $headerProbe = Join-Path $tbbRoot "include\tbb\tbb.h"

    try {
        if (-not (Test-Path $headerProbe)) {
            New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
            $zipPath = Join-Path $env:TEMP "oneapi-tbb-$onetbbVersion-win.zip"
            Write-Info "Downloading $onetbbUrl"
            Invoke-WebRequest -Uri $onetbbUrl -OutFile $zipPath -UseBasicParsing

            $actualHash = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($actualHash -ne $onetbbSha256) {
                throw "oneTBB SHA256 mismatch: expected $onetbbSha256, got $actualHash"
            }

            Write-Info "Extracting oneTBB to $installRoot"
            Expand-Archive -Path $zipPath -DestinationPath $installRoot -Force
            Remove-Item $zipPath -Force
        } else {
            Write-Info "oneTBB already present at $tbbRoot"
        }

        if (-not (Test-Path $headerProbe)) {
            throw "oneTBB header not found after install: $headerProbe"
        }

        $tbbLib = Join-Path $tbbRoot "lib\intel64\vc14"
        $tbbRedist = Join-Path $tbbRoot "redist\intel64\vc14"
        $tbbCmake = Join-Path $tbbRoot "lib\cmake\tbb"
        if (-not (Test-Path (Join-Path $tbbLib "tbb12.lib"))) {
            throw "oneTBB import library not found under $tbbLib"
        }

        $env:TBB_ROOT = $tbbRoot
        $env:TBBROOT = $tbbRoot
        $env:TBB_DIR = $tbbCmake
        if ($env:CMAKE_PREFIX_PATH) {
            $env:CMAKE_PREFIX_PATH = "$tbbRoot;$env:CMAKE_PREFIX_PATH"
        } else {
            $env:CMAKE_PREFIX_PATH = $tbbRoot
        }
        $env:Path = "$tbbRedist;$tbbLib;$env:Path"

        if ($env:GITHUB_ENV) {
            Add-Content -Path $env:GITHUB_ENV -Value "TBB_ROOT=$tbbRoot"
            Add-Content -Path $env:GITHUB_ENV -Value "TBBROOT=$tbbRoot"
            Add-Content -Path $env:GITHUB_ENV -Value "TBB_DIR=$tbbCmake"
            Add-Content -Path $env:GITHUB_ENV -Value "CMAKE_PREFIX_PATH=$($env:CMAKE_PREFIX_PATH)"
        }
        if ($env:GITHUB_PATH) {
            Add-Content -Path $env:GITHUB_PATH -Value $tbbRedist
            Add-Content -Path $env:GITHUB_PATH -Value $tbbLib
        }

        Write-Success "oneTBB $onetbbVersion installed at $tbbRoot"
    } catch {
        Write-Error "Failed to install oneTBB: $_"
        exit 1
    }
}

# CUDA Toolkit (optional)
if ($WithCuda) {
    Write-Info "Installing CUDA Toolkit..."
    $installCudaScript = Join-Path $PSScriptRoot "install-cuda-windows.ps1"
    if (Test-Path $installCudaScript) {
        & $installCudaScript
    } else {
        Write-Error "install-cuda-windows.ps1 not found next to install-deps-windows.ps1"
        exit 1
    }
}

# Python dependencies
Write-Info "Installing Python dependencies..."
try {
    python -m pip install --upgrade pip setuptools wheel | Out-Null
    python -m pip install colorama==0.4.6 psutil==6.1.1 | Out-Null
    Write-Success "Python dependencies installed"
} catch {
    Write-Warning "Failed to install Python dependencies: $_"
}

# Refresh environment variables
Write-Info "Refreshing environment variables..."
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
if ($WithTbb -and $env:TBB_ROOT) {
    $tbbLib = Join-Path $env:TBB_ROOT "lib\intel64\vc14"
    $tbbRedist = Join-Path $env:TBB_ROOT "redist\intel64\vc14"
    $env:Path = "$tbbRedist;$tbbLib;$env:Path"
}

# Verify installations
Write-Info "Verifying installations..."

$tools = @("cmake", "ninja", "git", "python", "clang", "buildcache")
foreach ($tool in $tools) {
    if (Get-Command $tool -ErrorAction SilentlyContinue) {
        Write-Success "$tool is available"
    } else {
        Write-Warning "$tool is not available in PATH"
    }
}

Write-Success "Windows dependency installation completed!"
Write-Info "You can now build XSigma using: python Scripts/setup.py ninja clang config build test"
