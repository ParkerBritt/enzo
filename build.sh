#!/usr/bin/env bash

# Usage: ./build.sh [Debug|Release|RelWithDebInfo]   (default: Debug)
BUILD_TYPE="${1:-Debug}"

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -G Ninja ..
ninja
