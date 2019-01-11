#!/bin/bash

error_exit() {
	echo "$1" 1>&2
	exit 1
}

Build() {
	CMakeBuildDir="${RootDir}/out/cmake/build/${Configuration}"
	CMakeInstallDir="${RootDir}/out/cmake/install/${Configuration}"
	mkdir -p "${CMakeBuildDir}" || error_exit "mkdir ${CMakeBuildDir} failed"
	(cd "${CMakeBuildDir}" && cmake -DCMAKE_INSTALL_PREFIX:PATH="${CMakeInstallDir}" -DCMAKE_BUILD_TYPE=${Configuration} "${RootDir}" || error_exit "cmake failed")
	cmake --build "${CMakeBuildDir}" --config ${Configuration} || error_exit "cmake build failed"
}

RunTests() {
	CTEST_OUTPUT_ON_FAILURE=1 cmake --build "${CMakeBuildDir}" --config ${Configuration} --target test || error_exit "cmake build failed"
	cat ${CMakeBuildDir}/Testing/Temporary/LastTest.log || error_exit "test log missing"
}

Install() {
	cmake --build "${CMakeBuildDir}" --target install || error_exit "cmake install failed"
}

Clean() {
	rm -f -r -v "${RootDir}/out" || error_exit "rm ${RootDir}/out failed"
}

Coverage() {
		Configuration=Debug
		Build

		Name="${CMakeBuildDir}/TestsCoverage"

		lcov -directory "${CMakeBuildDir}" --zerocounters || error_exit "lcov init"
		lcov -c -i -d "${CMakeBuildDir}" -o "${Name}.Base" || error_exit "lcov base"
		RunTests
		lcov -directory "${CMakeBuildDir}" --capture --output-file "${Name}.Info" || error_exit "lcov info"
		lcov -a "${Name}.Base" -a "${Name}.Info" --output-file "${Name}.Total" || error_exit "lcov total"
		lcov --remove "${Name}.Total" '/usr/include/*' '*/boost/*' '*/Tests/*' --output-file "${Name}.info.cleaned" || error_exit "lcov remove"
		genhtml -o "${Name}" "${Name}.info.cleaned" || error_exit "genhtml"
		echo "html coverage generated at: ${CMakeBuildDir}/TestsCoverage/index.html"
}

RootDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
Configuration=Release

clear
echo "go $1..."
case "$1" in

	"")
		;&
	"build")
		Build
		RunTests
		Install
		;;
	coverage)
		Coverage
		;;
	clean)
		Clean
		;;
	*)
		error_exit "Usage: $0 {build*|clean}"
esac


# todo, clean
