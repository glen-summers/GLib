cmake_minimum_required(VERSION 3.10.2)

include(../common.cmake)
SetCpp17()

add_library(GLib STATIC
	filelogger.cpp
	log.cpp
	LogManager.cpp
)

target_include_directories(GLib PUBLIC ../include)

# move to macro?
if (UNIX)
target_link_libraries(GLib stdc++fs)
endif (UNIX)

install(TARGETS GLib
	ARCHIVE DESTINATION lib
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
