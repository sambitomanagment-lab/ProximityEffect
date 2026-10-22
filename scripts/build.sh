#!/usr/bin/env bash
# Configure, build, test and package Proximity Effect on macOS or Linux.
#   scripts/build.sh            -> build/ + dist/
#   JUCE_PATH=/path/JUCE scripts/build.sh   uses a local JUCE instead of downloading it
set -euo pipefail
cd "$(dirname "$0")/.."

args=(-DCMAKE_BUILD_TYPE=Release -DPE_BUILD_TESTS=ON)
[[ -n "${JUCE_PATH:-}" ]] && args+=(-DJUCE_PATH="$JUCE_PATH")
[[ "$(uname)" == "Darwin" ]] && args+=(-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64")

cmake -B build "${args[@]}"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
scripts/package.sh
