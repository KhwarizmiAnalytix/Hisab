# Install the CUDA toolkit on Windows so MEMORY_GPU_BACKEND=cuda /
# --gpu_backend=cuda can configure and compile. GitHub-hosted Windows runners
# have no NVIDIA GPU or driver; device-dependent tests skip at runtime (see
# TestCudaCachingAllocator's cuda_device_available() gate).
#
# Uses NVIDIA's network installer directly (not the Chocolatey "cuda"
# package, which has a history of being stale/unreliable on hosted runners).
# Mirrors install-cuda-ubuntu.sh's role for the Linux CI job.

param(
    [string]$Version = "12.6.2"
)

$ErrorActionPreference = "Stop"

if (Get-Command nvcc -ErrorAction SilentlyContinue) {
    Write-Host "nvcc already available: $(& nvcc --version | Select-String release)"
    exit 0
}

$installerUrl = "https://developer.download.nvidia.com/compute/cuda/$Version/network_installers/cuda_${Version}_windows_network.exe"
$installerPath = Join-Path $env:TEMP "cuda_installer.exe"

Write-Host "Downloading CUDA $Version network installer from $installerUrl ..."
# The default progress-bar rendering makes Invoke-WebRequest extremely slow
# for large files in non-interactive CI hosts.
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri $installerUrl -OutFile $installerPath -TimeoutSec 600

# Full silent install ("-s" with no subpackage names): the network installer
# only fetches the components it installs, and picking an exact minimal
# subpackage list (nvcc_X.Y, cudart_X.Y, cupti_X.Y, ...) risks a silently
# incomplete install if a name is wrong. Prefer correctness over CI runtime
# here; narrow this once verified against real CI runs.
Write-Host "Running silent CUDA install..."
$proc = Start-Process -FilePath $installerPath -ArgumentList "-s" -Wait -PassThru -NoNewWindow
if ($proc.ExitCode -ne 0) {
    throw "CUDA installer exited with code $($proc.ExitCode)"
}

$cudaMajorMinor = ($Version -split '\.')[0..1] -join '.'
$cudaRoot = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v$cudaMajorMinor"
if (-not (Test-Path $cudaRoot)) {
    throw "CUDA install did not produce the expected directory: $cudaRoot"
}

$cudaBin = Join-Path $cudaRoot "bin"
$cudaPathVarName = "CUDA_PATH_V{0}" -f ($cudaMajorMinor -replace '\.', '_')
Add-Content -Path $env:GITHUB_PATH -Value $cudaBin
Add-Content -Path $env:GITHUB_ENV -Value "CUDA_PATH=$cudaRoot"
Add-Content -Path $env:GITHUB_ENV -Value "$cudaPathVarName=$cudaRoot"

$env:Path = "$cudaBin;$env:Path"
& "$cudaBin\nvcc.exe" --version
Write-Host "CUDA Toolkit $Version installed at $cudaRoot"
