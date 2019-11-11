cmake_minimum_required(VERSION 3.14)

include(../cmake/common.cmake)

#todo pch
set(SOURCES Main.cpp TestApp.rc)

include_directories(../include)

add_executable(TestApp WIN32 ${SOURCES})

target_link_libraries(TestApp GLib)

install(TARGETS TestApp
	RUNTIME DESTINATION bin
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
