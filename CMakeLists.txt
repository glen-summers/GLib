cmake_minimum_required(VERSION 3.10.2)

function(R_SEARCH_ABOVE search_path search_item return_value)
while(true)
	if (EXISTS "${search_path}/${search_item}")
		break()
	else()
		get_filename_component(parent_dir ${search_path} DIRECTORY)
		if (${parent_dir} STREQUAL ${search_path})
			break()
		endif()
		set(search_path ${parent_dir})
	endif()
endwhile()
set(${return_value} ${search_path} PARENT_SCOPE)
endfunction(R_SEARCH_ABOVE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BOOST_VER 1_68_0)

if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
	foreach(CompilerFlag ${CompilerFlags})
	  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endif (WIN32)

R_SEARCH_ABOVE(${CMAKE_CURRENT_SOURCE_DIR} ExternalDependencies/boost_${BOOST_VER}/Jamroot EXT_DIR)
if(NOT EXT_DIR)
	message(FATAL_ERROR "Could not find ExternalDependencies")
endif()

set(BOOST_DIR ${EXT_DIR}/ExternalDependencies/boost_${BOOST_VER})
message("boost dir = ${BOOST_DIR}")
link_directories(${BOOST_DIR}/stage/lib)

project(GLib VERSION 1.0.0 DESCRIPTION "GLib library")

add_library(GLib STATIC
	GLib/filelogger.cpp
	GLib/log.cpp
	GLib/LogManager.cpp
)

add_executable(AllTests
	Tests/Main.cpp
	Tests/FlogTests.cpp
	Tests/FormatterTests.cpp
)

if (UNIX)
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} stdc++fs")
#message("CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
target_link_libraries(${PROJECT_NAME} stdc++fs)
endif (UNIX)

include_directories(${BOOST_DIR} include)

target_link_libraries(AllTests GLib)

install(TARGETS AllTests GLib
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)