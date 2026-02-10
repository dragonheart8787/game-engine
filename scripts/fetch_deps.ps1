param(
  [string]$VendorDir = "third_party",
  [string]$Glm = "0.9.9.8",
  [string]$Json = "v3.11.3"
)

$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path $VendorDir | Out-Null

$glmPath = Join-Path $VendorDir "glm"
$jsonPath = Join-Path $VendorDir "json"

if (!(Test-Path (Join-Path $glmPath ".git"))) {
  if (Test-Path $glmPath) { Remove-Item -Recurse -Force $glmPath }
  git clone --depth 1 --branch $Glm https://github.com/g-truc/glm.git $glmPath
}

if (!(Test-Path (Join-Path $jsonPath ".git"))) {
  if (Test-Path $jsonPath) { Remove-Item -Recurse -Force $jsonPath }
  git clone --depth 1 --branch $Json https://github.com/nlohmann/json.git $jsonPath
}

Write-Host "Dependencies fetched into $VendorDir"
