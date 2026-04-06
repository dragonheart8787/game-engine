#Requires -Version 5.1
# Build + CTest from game-engine. ASCII-only for PS 5.1 encoding.
# Do not paste lines starting with ">>" (that is the PS continuation prompt, not a command).
param(
  [switch]$PreferKitwareCMake
)

$ErrorActionPreference = "Stop"

function Get-CMakeBinDirectory {
  $raw = $env:CMAKE_BIN
  if (-not $raw) {
    return $null
  }
  $raw = $raw.Trim().TrimEnd('\', '/')
  if (-not (Test-Path -LiteralPath $raw)) {
    return $null
  }
  $item = Get-Item -LiteralPath $raw
  if ($item.PSIsContainer) {
    $tryDirs = @(
      (Join-Path $raw "cmake.exe"),
      (Join-Path $raw "bin\cmake.exe")
    )
    foreach ($exe in $tryDirs) {
      if (Test-Path -LiteralPath $exe) {
        return (Split-Path -Parent $exe)
      }
    }
    return $null
  }
  if ($item.Name -ieq "cmake.exe") {
    return $item.Directory.FullName
  }
  return $null
}

function Find-CMakeBinDirectory {
  if ($env:CMAKE_BIN) {
    $fromEnv = Get-CMakeBinDirectory
    if ($fromEnv) {
      return $fromEnv
    }
    Write-Host "[WeaveBound] WARNING: CMAKE_BIN is set but cmake.exe was not found under: $($env:CMAKE_BIN)" -ForegroundColor Yellow
    Write-Host "[WeaveBound] Tried: <dir>\cmake.exe and <dir>\bin\cmake.exe" -ForegroundColor DarkYellow
  }
  $exe = Get-Command cmake.exe -ErrorAction SilentlyContinue
  if ($exe -and $exe.Source) {
    return (Split-Path -Parent $exe.Source)
  }
  $candidates = @(
    (Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"),
    (Join-Path ${env:ProgramFiles(x86)} "CMake\bin\cmake.exe"),
    (Join-Path $env:LocalAppData "Programs\CMake\bin\cmake.exe")
  )
  foreach ($p in $candidates) {
    if ($p -and (Test-Path -LiteralPath $p)) {
      return (Split-Path -Parent $p)
    }
  }
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path -LiteralPath $vswhere) {
    $inst = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($inst) {
      $vscmake = Join-Path $inst "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
      if (Test-Path -LiteralPath $vscmake) {
        return (Split-Path -Parent $vscmake)
      }
    }
  }
  return $null
}

function Resolve-CMakeRootAvoidingVsKitwareMix {
  param(
    [string]$RootDir,
    [bool]$ForceKitware
  )
  $kitwareExe = Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"
  $hasKitware = Test-Path -LiteralPath $kitwareExe
  $kitwareParent = Split-Path -Parent $kitwareExe
  if ($ForceKitware) {
    if ($hasKitware) {
      Write-Host "[WeaveBound] -PreferKitwareCMake: using $kitwareExe" -ForegroundColor Cyan
      return $kitwareParent
    }
    Write-Host "[WeaveBound] -PreferKitwareCMake ignored; not found: $kitwareExe" -ForegroundColor Yellow
    return $RootDir
  }
  if ($env:WB_FORCE_VS_CMAKE -eq '1') {
    return $RootDir
  }
  $cmakeExePath = Join-Path $RootDir "cmake.exe"
  if (-not $hasKitware) {
    return $RootDir
  }
  if ($cmakeExePath -notmatch '(?i)CommonExtensions\\Microsoft\\CMake\\CMake\\bin') {
    return $RootDir
  }
  Write-Host "[WeaveBound] VS-bundled CMake can load the wrong Modules tree if Kitware CMake is also installed." -ForegroundColor DarkYellow
  Write-Host "[WeaveBound] Using Kitware CMake: $kitwareExe" -ForegroundColor Cyan
  Write-Host "[WeaveBound] Or clear CMAKE_BIN and run again; to force VS cmake set WB_FORCE_VS_CMAKE=1" -ForegroundColor DarkGray
  return $kitwareParent
}

$cmakeRoot = Find-CMakeBinDirectory
if (-not $cmakeRoot) {
  Write-Host ""
  Write-Host "[WeaveBound] cmake.exe not found. Set CMAKE_BIN to the folder that contains cmake.exe or to cmake.exe itself." -ForegroundColor Yellow
  Write-Host '  Example: $env:CMAKE_BIN = "C:\Program Files\CMake\bin\cmake.exe"' -ForegroundColor Gray
  Write-Host "Install: https://cmake.org/download/" -ForegroundColor Cyan
  Write-Host ""
  exit 127
}

$cmakeRoot = Resolve-CMakeRootAvoidingVsKitwareMix -RootDir $cmakeRoot -ForceKitware:$PreferKitwareCMake

$cmake = Join-Path $cmakeRoot "cmake.exe"
$ctest = Join-Path $cmakeRoot "ctest.exe"
if (-not (Test-Path -LiteralPath $ctest)) {
  $ctest = "ctest"
}

$EngineRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location -LiteralPath $EngineRoot
$BuildDir = Join-Path $EngineRoot "build"

# Stale env sometimes points Modules at another CMake install.
Remove-Item Env:CMAKE_ROOT -ErrorAction SilentlyContinue

Write-Host "[WeaveBound] CMake: $cmake" -ForegroundColor DarkGray

$cmakeCache = Join-Path $BuildDir "CMakeCache.txt"
$needsConfigure = (-not (Test-Path -LiteralPath $BuildDir)) -or (-not (Test-Path -LiteralPath $cmakeCache))
if ($needsConfigure) {
  if ((Test-Path -LiteralPath $BuildDir) -and -not (Test-Path -LiteralPath $cmakeCache)) {
    Write-Host "[WeaveBound] build\ exists but CMakeCache.txt missing; re-running cmake configure." -ForegroundColor DarkYellow
  }
  & $cmake -S . -B build
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
& $cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $ctest --test-dir build -C Release
exit $LASTEXITCODE
