Param(
  [Parameter(Mandatory=$true)][string]$BuildDir,
  [Parameter(Mandatory=$false)][string]$Config = "Release",
  [Parameter(Mandatory=$false)][string]$OutDir = "dist/windows"
)

$ErrorActionPreference = "Stop"

function Copy-Dir($src, $dst) {
  if (Test-Path $dst) { Remove-Item -Recurse -Force $dst }
  New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null
  Copy-Item -Recurse -Force $src $dst
}

$artefacts = Join-Path $BuildDir ("MelodyForgePro_artefacts\" + $Config)
if (-not (Test-Path $artefacts)) {
  throw "Artefacts directory not found: $artefacts"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$vst3 = Join-Path $artefacts "VST3\MelodyForgePro.vst3"
if (-not (Test-Path $vst3)) {
  throw "VST3 not found: $vst3"
}
Copy-Dir $vst3 (Join-Path $OutDir "MelodyForgePro.vst3")

$standalone = Join-Path $artefacts "Standalone\MelodyForgePro.exe"
if (-not (Test-Path $standalone)) {
  throw "Standalone exe not found: $standalone"
}
Copy-Item -Force $standalone (Join-Path $OutDir "MelodyForgePro.exe")

Write-Host "Packaged Windows outputs to $OutDir"

