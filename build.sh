#!/bin/bash

# This script's directory
SCRIPT_DIR=$(dirname $(realpath $0))
BUILD_MODE=Debug

# Qt directory so CMake can find it
QT_CMAKE_PATH=${BR_QT_PATH}/lib/cmake/

set -ev
cd ${SCRIPT_DIR}
git submodule update --init
mkdir -p build
cmake -S . -B build \
	-DCMAKE_PREFIX_PATH=${QT_CMAKE_PATH} \

echo "Using ${nproc:-$(nproc)} jobs"
cmake --build build -j ${nproc}
