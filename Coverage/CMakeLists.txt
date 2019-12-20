cmake_minimum_required(VERSION 3.12.4)

include(../cmake/common.cmake)

#todo pch
set(SOURCES Main.cpp Coverage.cpp HtmlReport.cpp)

include_directories(../include)

add_executable(Coverage ${SOURCES})

target_link_libraries(Coverage GLib)

install(TARGETS Coverage
	RUNTIME DESTINATION bin
	CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
