cmake_minimum_required(VERSION 3.10.2)

set(BOOST_VER 1_68_0)
include(../cmake/common.cmake)

# not working...
#if(UNIX)
#include(FindICU)
#if(ICU_FOUND)
#	include_directories(${ICU_INCLUDE_DIRS})
#	link_libraries(${ICU_DATA_LIBRARIES} ${ICU_I18N_LIBRARIES} ${ICU_LIBRARIES})
#endif(ICU_FOUND)
#endif()

R_SEARCH_ABOVE(${CMAKE_CURRENT_SOURCE_DIR} ExternalDependencies/boost_${BOOST_VER} Jamroot BOOST_DIR)
if(NOT BOOST_DIR)
	message(FATAL_ERROR "Could not find ExternalDependencies")
endif()

message("boost = ${BOOST_DIR}")
include_directories(${BOOST_DIR})
link_directories(${BOOST_DIR}/stage/lib)


set(SOURCES Main.cpp
	FlogTests.cpp
	FormatterTests.cpp
	ConverterTests.cpp
	ScopeTests.cpp
	CheckedCastTests.cpp
	SplitTests.cpp)

if(WIN32)
	set(SOURCES ${SOURCES} WinTests.cpp)
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
