#!/bin/bash

error_exit() {
	echo "$1" 1>&2
	exit 1
}

Build() {
	clear
	mkdir -p "${CMakeBuildDir}" || error_exit "mkdir ${CMakeBuildDir} failed"
	(cd "${CMakeBuildDir}" && cmake -DCMAKE_INSTALL_PREFIX:PATH="${CMakeInstallDir}" "${RootDir}" || error_exit "cmake failed")
	cmake --build "${CMakeBuildDir}" || error_exit "cmake build failed"
	CTEST_OUTPUT_ON_FAILURE=1 cmake --build "${CMakeBuildDir}" --target test || error_exit "cmake build failed"
	cmake --build "${CMakeBuildDir}" --target install || error_exit "cmake install failed"
}

Clean() {
	rm -f -r -v "${RootDir}/out" || error_exit "rm ${RootDir}/out failed"
}

RootDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
CMakeInstallDir="${RootDir}/out/cmake/install"
CMakeBuildDir="${RootDir}/out/cmake/build"

echo "go $1..."
case "$1" in
	"build")
		;&
	"")
		Build
		;;
	clean)
		Clean
		;;
	*)
		error_exit "Usage: $0 {build*|clean}"
esac


# todo, clean
