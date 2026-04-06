#Requires -Version 5.1
# All user-visible strings are ASCII so Windows PowerShell 5.1 parses this file
# correctly when the file is UTF-8 without BOM (common default for editors).
<#
.SYNOPSIS
  Download/install WeaveBound deps: CMake (winget), Vulkan SDK installer, pip packages.
.PARAMETER SkipWinget
  Skip winget; only download Vulkan installer and run pip.
.PARAMETER VulkanSdkVersion
  LunarG SDK version, e.g. 1.3.296.0
#>
param(
  [switch]$SkipWinget,
  [string]$VulkanSdkVersion = "1.3.296.0"
)

$ErrorActionPreference = "Stop"
$GameEngineRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$WorkspaceRoot = Split-Path -Parent $GameEngineRoot
$ThirdParty = Join-Path $GameEngineRoot "tools\third_party"
New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null

function Test-Cmd {
  param([string]$Name)
  return [bool](Get-Command -Name $Name -ErrorAction SilentlyContinue)
}

if (-not $SkipWinget) {
  if (Test-Cmd -Name "winget") {
    Write-Host "==> winget: CMake"
    try {
      winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements | Out-Null
    } catch {
      Write-Host "winget CMake: $_"
    }
    Write-Host "==> winget: Python 3.12"
    try {
      winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements | Out-Null
    } catch {
      Write-Host "winget Python: $_"
    }
  } else {
    Write-Warning "winget not found. Install CMake and Python manually: https://cmake.org/download/"
  }
}

$VulkanUrl = "https://sdk.lunarg.com/sdk/download/$VulkanSdkVersion/windows/VulkanSDK-$VulkanSdkVersion-Installer.exe"
$VulkanExe = Join-Path $ThirdParty "VulkanSDK-Installer.exe"
Write-Host "==> Download Vulkan SDK installer:"
Write-Host "    $VulkanUrl"
try {
  Invoke-WebRequest -Uri $VulkanUrl -OutFile $VulkanExe -UseBasicParsing
} catch {
  Write-Warning "Download failed (wrong version?). Get SDK from https://vulkan.lunarg.com/sdk/home#windows"
  throw $_
}

Write-Host ""
Write-Host "==> Next: run the Vulkan installer GUI. Enable PATH / VULKAN_SDK."
Write-Host "    Optional silent (if your installer supports it):"
Write-Host "    Start-Process -FilePath '$VulkanExe' -ArgumentList '--al','--am','--accept-licenses','--confirm-command','install' -Wait"
Write-Host ""

$Open = Read-Host "Launch Vulkan installer now? [Y/n]"
if ($Open -ne "n" -and $Open -ne "N") {
  Start-Process -FilePath $VulkanExe -Wait
}

Write-Host "==> pip: game-engine (editable) + pytest"
Push-Location $GameEngineRoot
python -m pip install --upgrade pip
python -m pip install --no-warn-script-location -e .
python -m pip install --no-warn-script-location pytest
Pop-Location

try {
  $userScripts = python -c "import pathlib, site; print(pathlib.Path(site.USER_BASE) / 'Scripts')" 2>$null
  if ($userScripts -and (Test-Path -LiteralPath $userScripts)) {
    Write-Host ""
    Write-Host "If pip warned Scripts is not on PATH, add this folder to User PATH:"
    Write-Host "  $userScripts"
    Write-Host "(Settings -> System -> About -> Advanced -> Environment Variables)"
  }
} catch {
  # ignore
}

$EidrixReq = Join-Path $WorkspaceRoot "eidrix_mvp\requirements.txt"
if (Test-Path -LiteralPath $EidrixReq) {
  Write-Host "==> pip: eidrix_mvp"
  Push-Location (Join-Path $WorkspaceRoot "eidrix_mvp")
  python -m pip install --no-warn-script-location -r requirements.txt
  try {
    python -m pip install --no-warn-script-location -e $GameEngineRoot | Out-Null
  } catch {
    # ignore
  }
  if (Test-Path -LiteralPath "requirements_3d.txt") {
    Write-Host "==> pip: requirements_3d.txt (pygame skipped on Python 3.14+ by default)"
    try {
      python -m pip install --no-warn-script-location -r requirements_3d.txt
    } catch {
      Write-Warning "requirements_3d.txt had errors; check output above."
    }
  }
  Pop-Location
}

$CompileScript = Join-Path $GameEngineRoot "scripts\bootstrap\compile_shaders.ps1"
Write-Host ""
Write-Host "Next steps:"
Write-Host "  - Restart terminal; verify cmake, python, VULKAN_SDK"
Write-Host "  - Shaders: powershell -ExecutionPolicy Bypass -File $CompileScript"
Write-Host "  - Build:   cd $GameEngineRoot"
Write-Host "             cmake -S . -B build"
Write-Host "             cmake --build build --config Release"
Write-Host ""
