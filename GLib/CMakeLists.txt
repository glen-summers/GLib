cmake_minimum_required(VERSION 3.10.2)

include(../common.cmake)

add_library(GLib STATIC
	filelogger.cpp
	log.cpp
	LogManager.cpp
)

target_include_directories(GLib PUBLIC ../include)

AddStdLinkage(GLib)

install(TARGETS GLib
	ARCHIVE DESTINATION lib
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
