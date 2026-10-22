# Zips the built Windows plugin into dist\ProximityEffect-Windows-x64.zip. Run after a Release build.
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

$art = "build\ProximityEffect_artefacts\Release"
$name = "ProximityEffect-Windows-x64"
$stage = "dist\stage"
$version = (Select-String -Path CMakeLists.txt -Pattern 'project\(ProximityEffect VERSION ([0-9.]+)\)').Matches[0].Groups[1].Value

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item -Recurse "$art\VST3\Proximity Effect.vst3" $stage
Copy-Item "$art\Standalone\Proximity Effect.exe" $stage
Copy-Item "docs\INSTALL.md" $stage
Set-Content -Path "$stage\VERSION.txt" -Value $version

if (Test-Path "dist\$name.zip") { Remove-Item -Force "dist\$name.zip" }
Compress-Archive -Path "$stage\*" -DestinationPath "dist\$name.zip"
Remove-Item -Recurse -Force $stage
Write-Host "Package: dist\$name.zip"
