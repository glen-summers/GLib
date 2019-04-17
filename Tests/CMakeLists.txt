cmake_minimum_required(VERSION 3.10.2)

set(BOOST_VER boost_1_69_0_TEST)
set(BOOST_SIG ExternalDependencies/${BOOST_VER})
set(BOOST_ITEM boost/test/unit_test.hpp)

include(../cmake/common.cmake)

# not working...
#if(UNIX)
#include(FindICU)
#if(ICU_FOUND)
#	include_directories(${ICU_INCLUDE_DIRS})
#	link_libraries(${ICU_DATA_LIBRARIES} ${ICU_I18N_LIBRARIES} ${ICU_LIBRARIES})
#endif(ICU_FOUND)
#endif()

if(UNIX AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
	link_libraries(gcov)
	message("Set config for coverage")
endif()

R_SEARCH_ABOVE(${CMAKE_CURRENT_SOURCE_DIR} ${BOOST_SIG} ${BOOST_ITEM} BOOST_DIR)
if(NOT BOOST_DIR)
	message(FATAL_ERROR "Could not find ExternalDependencies")
endif()

message("boost = ${BOOST_DIR}")
include_directories(${BOOST_DIR})
link_directories(${BOOST_DIR}/stage/lib)

set(SOURCES Main.cpp
	CheckedCastTests.cpp
	ConverterTests.cpp
	FlogTests.cpp
	FormatterTests.cpp
	IcuUtilsTests.cpp
	NoCaseTests.cpp
	ScopeTests.cpp
	SplitTests.cpp
	TypeFilterTests.cpp
)

if(WIN32)
	add_compile_definitions(BOOST_ALL_NO_LIB)
	set(SOURCES ${SOURCES} WinTests.cpp)
	set(SOURCES ${SOURCES} ComPtrTests.cpp)
endif(WIN32)

add_executable(Tests ${SOURCES})

add_subdirectory(../GLib GLib)

target_link_libraries(Tests GLib)
if(UNIX) #hack: just add libs that weren't found by FindICU
	target_link_libraries(Tests icui18n icuuc)
endif()

add_test(NAME Tests COMMAND Tests)

install(TARGETS Tests
	RUNTIME DESTINATION bin
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
