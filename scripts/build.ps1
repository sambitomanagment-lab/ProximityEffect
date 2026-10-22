# Configure, build, test and package Proximity Effect on Windows (x64).
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1
#   $env:JUCE_PATH = "C:\path\JUCE" to use a local JUCE instead of downloading it
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

$cmakeArgs = @("-B", "build", "-A", "x64", "-DPE_BUILD_TESTS=ON")
if ($env:JUCE_PATH) { $cmakeArgs += "-DJUCE_PATH=$env:JUCE_PATH" }

cmake @cmakeArgs; if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build --config Release --parallel; if ($LASTEXITCODE) { exit $LASTEXITCODE }
ctest --test-dir build -C Release --output-on-failure; if ($LASTEXITCODE) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot "package.ps1")
