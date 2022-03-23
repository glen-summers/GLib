cmake_minimum_required(VERSION 3.12.4)

include(../cmake/common.cmake)

add_library(GLib STATIC
	FileLogger.cpp
	Log.cpp
	LogManager.cpp
)

target_include_directories(GLib PUBLIC ../include)

AddStdLinkage(GLib)

install(TARGETS GLib
	ARCHIVE DESTINATION lib
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
