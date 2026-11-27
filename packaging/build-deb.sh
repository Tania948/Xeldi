#!/usr/bin/env bash
# Construye el paquete .deb de Xeldi (Ubuntu 22.04+ / Debian 12+).
#   sudo apt install build-essential cmake ninja-build qt6-base-dev nlohmann-json3-dev zlib1g-dev dpkg-dev
#   ./packaging/build-deb.sh
#   sudo apt install ./build-deb/xeldi_*.deb
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build-deb -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-deb
ctest --test-dir build-deb --output-on-failure
(cd build-deb && cpack -G DEB)
ls -1 build-deb/*.deb
