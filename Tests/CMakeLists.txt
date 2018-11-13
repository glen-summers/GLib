cmake_minimum_required(VERSION 3.10.2)

set(BOOST_VER 1_68_0)
include(../common.cmake)

R_SEARCH_ABOVE(${CMAKE_CURRENT_SOURCE_DIR} ExternalDependencies/boost_${BOOST_VER} Jamroot BOOST_DIR)
if(NOT BOOST_DIR)
	message(FATAL_ERROR "Could not find ExternalDependencies")
endif()

message("boost = ${BOOST_DIR}")
include_directories(${BOOST_DIR})
link_directories(${BOOST_DIR}/stage/lib)

add_executable(AllTests
	Main.cpp
	FlogTests.cpp
	FormatterTests.cpp
)

add_subdirectory(../GLib GLib)

target_link_libraries(AllTests GLib)

install(TARGETS AllTests
	RUNTIME DESTINATION bin
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)

set(UNIT_TEST AllTests)
add_custom_command(
     TARGET ${UNIT_TEST}
     COMMENT "Run tests"
     POST_BUILD 
     COMMAND ${UNIT_TEST}
)