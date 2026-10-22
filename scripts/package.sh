#!/usr/bin/env bash
# Zips the built plugins into dist/. Run after a Release build.
set -euo pipefail
cd "$(dirname "$0")/.."

art="build/ProximityEffect_artefacts/Release"
version="$(sed -n 's/^project(ProximityEffect VERSION \([0-9.]*\)).*/\1/p' CMakeLists.txt)"
rm -rf dist/stage && mkdir -p dist/stage

case "$(uname)" in
  Darwin)
    name="ProximityEffect-macOS-Universal"
    cp -R "$art/VST3/Proximity Effect.vst3" "$art/AU/Proximity Effect.component" "$art/Standalone/Proximity Effect.app" dist/stage/
    # Ad-hoc signature: required for Apple Silicon to load the binaries at all.
    for b in dist/stage/*; do codesign --force --deep --sign - "$b"; done
    archs="$(lipo -archs "dist/stage/Proximity Effect.vst3/Contents/MacOS/Proximity Effect")"
    echo "Architectures: $archs"
    ;;
  Linux)
    name="ProximityEffect-Linux-x64"
    cp -R "$art/VST3/Proximity Effect.vst3" "$art/Standalone/Proximity Effect" dist/stage/
    ;;
  *) echo "Use scripts/package.ps1 on Windows"; exit 1 ;;
esac

cp docs/INSTALL.md dist/stage/INSTALL.md
echo "$version" > dist/stage/VERSION.txt
rm -f "dist/$name.zip"
(cd dist/stage && if [[ "$(uname)" == "Darwin" ]]; then ditto -c -k --keepParent . "../$name.zip"; else zip -qry "../$name.zip" .; fi)
rm -rf dist/stage
echo "Package: dist/$name.zip"
