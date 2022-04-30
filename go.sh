#!/bin/bash

error_exit() {
	echo "$1" 1>&2
	exit 1
}

Perms() {
	sudo locale-gen en_GB.UTF-8 || error_exit "locale-gen"
	sudo update-locale LANG=en_GB.UTF-8 || error_exit "update-locale"
	locale -a
}

Deps() {
	Url="https://github.com/glen-summers/BoostModularBuild/archive/v${BoostBuilderVer}.tar.gz"

	mkdir -p "${RootDir}/out/downloads/Deps" || error_exit "mkdir failed"
	pushd "${RootDir}/out/downloads/Deps" || error_exit "download failed"
	ls "${RootDir}/out/downloads/Deps"
	(wget -c ${Url} -O - | tar -xz) || error_exit "download failed"
	ls "${RootDir}/out/downloads/Deps"
	. "./BoostModularBuild-${BoostBuilderVer}/go.sh" build test || error_exit "module build failed"
}

Build() {
	CMakeBuildDir="${RootDir}/out/cmake/build/${Configuration}"
	CMakeInstallDir="${RootDir}/out/cmake/install/${Configuration}"
	mkdir -p "${CMakeBuildDir}" || error_exit "mkdir ${CMakeBuildDir} failed"
	(cd "${CMakeBuildDir}" && cmake -DCMAKE_INSTALL_PREFIX:PATH="${CMakeInstallDir}" -DCMAKE_BUILD_TYPE=${Configuration} "${RootDir}" || error_exit "cmake failed")
	cmake --build "${CMakeBuildDir}" --config ${Configuration} || error_exit "cmake build failed"
}

RunTests() {
	echo $LANG
	LANG=en_GB.UTF-8
	echo $LANG
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
		genhtml --demangle-cpp -o "${Name}" "${Name}.info.cleaned" || error_exit "genhtml"
		echo "html coverage generated at: ${Name}/index.html"
		ls ${Name}.info.cleaned
}

RootDir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
Configuration=Release
BoostBuilderVer=`cat ${RootDir}/boost.version`
echo Boost=${BoostBuilderVer}

echo "${RootDir}/go.sh $1..."
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
	deps)
		Deps
		;;
	perms)
		Perms
		;;
	*)
		error_exit "Usage: $0 {build*|clean}"
esac


# todo, clean
