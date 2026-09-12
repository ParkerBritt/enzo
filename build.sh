#!/usr/bin/env bash

# Usage: ./build.sh [Debug|Release|RelWithDebInfo] [--superbuild]   (default: Debug)
#
# --superbuild builds the pinned dependencies into build-super/deps first, then
# builds enzo against them. Without it enzo builds into build against whatever
# the system provides.
set -e

BUILD_TYPE=Debug
SUPERBUILD=OFF
BUILD_DIR=build

for arg in "$@"; do
    case "$arg" in
        --superbuild) SUPERBUILD=ON; BUILD_DIR=build-super ;;
        *) BUILD_TYPE="$arg" ;;
    esac
done

cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENZO_SUPERBUILD="$SUPERBUILD"
cmake --build "$BUILD_DIR"
