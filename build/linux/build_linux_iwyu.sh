#!/bin/bash
# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set -e

build_root=$(cd "$1" && pwd)
cd $build_root

build_folder=$build_root"/cmake_linux_iwyu"

# Set the default cores
CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
CC="clang" CXX="clang++" cmake $build_root -GNinja -DCMAKE_C_INCLUDE_WHAT_YOU_USE:UNINITIALIZED=include-what-you-use;-Xiwyu;--mapping_file=../deps/c-build-tools/iwyu/rules.imp; -DCMAKE_CXX_INCLUDE_WHAT_YOU_USE:UNINITIALIZED=include-what-you-use;-Xiwyu;--mapping_file=../deps/c-build-tools/iwyu/rules.imp; -Drun_unittests:bool=ON -Drun_int_tests:bool=ON
cmake --build .

# This needs to look for the Warning text in the build output

popd