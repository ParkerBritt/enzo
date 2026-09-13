#!/usr/bin/env bash

# Usage: ./build.sh [Debug|Release|RelWithDebInfo] [--system-deps]  (default: Debug)
#
# The pinned dependencies are built into build-super/deps first, then enzo is
# built against them. --system-deps skips that and builds into build against
# whatever the system provides.
set -e

BUILD_TYPE=Debug
SUPERBUILD=ON
BUILD_DIR=build-super

for arg in "$@"; do
    case "$arg" in
        --system-deps) SUPERBUILD=OFF; BUILD_DIR=build ;;
        *) BUILD_TYPE="$arg" ;;
    esac
done

cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENZO_SUPERBUILD="$SUPERBUILD"
cmake --build "$BUILD_DIR"
