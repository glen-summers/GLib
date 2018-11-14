#!/bin/bash

error_exit() {
	echo "$1" 1>&2
	exit 1
}

clear
cd "$( dirname "${BASH_SOURCE[0]}" )"

cmake . -DCMAKE_INSTALL_PREFIX:PATH="./install" || error_exit "cmake failed"
cmake --build . || error_exit "cmake build failed"
CTEST_OUTPUT_ON_FAILURE=1 cmake --build . --target test || error_exit "cmake build failed"
cmake --build . --target install || error_exit "cmake install failed"
