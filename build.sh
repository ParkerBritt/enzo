#!/usr/bin/env bash

# Usage: ./build.sh [Debug|Release|RelWithDebInfo]  (default: Debug)
#
# The pinned dependencies are built into build/deps first, then enzo is built
# against them in build/enzo-build.
set -e

BUILD_TYPE="${1:-Debug}"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build build
