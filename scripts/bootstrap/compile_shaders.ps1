#Requires -Version 5.1
# ASCII-only strings for Windows PowerShell 5.1 without UTF-8 BOM.
<#
.SYNOPSIS
  Compile engine/shaders/*.vert and *.frag to SPIR-V using Vulkan SDK glslc.
  Writes to engine/shaders/baked/ (for CMake when glslc is missing).
.PARAMETER OutDir
  Optional extra copy destination (e.g. build/Release/shaders)
#>
param(
  [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$EngineRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ShaderSrc = Join-Path $EngineRoot "engine\shaders"
$Baked = Join-Path $ShaderSrc "baked"
New-Item -ItemType Directory -Force -Path $Baked | Out-Null

if (-not $env:VULKAN_SDK) {
  Write-Error "Set VULKAN_SDK after installing Vulkan SDK, or run download_deps_windows.ps1"
}
$Glslc = Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
if (-not (Test-Path -LiteralPath $Glslc)) {
  Write-Error "glslc not found: $Glslc"
}

$Pairs = @(
  @{ Vert = "triangle.vert"; Frag = "triangle.frag" }
)
foreach ($p in $Pairs) {
  $vIn = Join-Path $ShaderSrc $p.Vert
  $fIn = Join-Path $ShaderSrc $p.frag
  $vOut = Join-Path $Baked ($p.Vert + ".spv")
  $fOut = Join-Path $Baked ($p.frag + ".spv")
  if (-not (Test-Path -LiteralPath $vIn)) { Write-Error "Missing $vIn" }
  if (-not (Test-Path -LiteralPath $fIn)) { Write-Error "Missing $fIn" }
  Write-Host "glslc $vIn -> $vOut"
  & $Glslc -o $vOut $vIn
  Write-Host "glslc $fIn -> $fOut"
  & $Glslc -o $fOut $fIn
}

$CompIn = Join-Path $ShaderSrc "trivial.comp"
$CompOut = Join-Path $Baked "trivial.comp.spv"
if (-not (Test-Path -LiteralPath $CompIn)) { Write-Error "Missing $CompIn" }
Write-Host "glslc $CompIn -> $CompOut"
& $Glslc -o $CompOut $CompIn

if ($OutDir) {
  $d = $OutDir
  New-Item -ItemType Directory -Force -Path $d | Out-Null
  foreach ($p in $Pairs) {
    Copy-Item (Join-Path $Baked ($p.Vert + ".spv")) (Join-Path $d ($p.Vert + ".spv")) -Force
    Copy-Item (Join-Path $Baked ($p.frag + ".spv")) (Join-Path $d ($p.frag + ".spv")) -Force
  }
  Copy-Item $CompOut (Join-Path $d "trivial.comp.spv") -Force
  Write-Host "Copied SPIR-V to $d"
}
Write-Host "Done. Baked shaders: $Baked"
