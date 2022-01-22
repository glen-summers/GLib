cmake_minimum_required(VERSION 3.12.4)

include(ExternalProject)

file (STRINGS "boost.version" BOOST_VER)

project(GLib VERSION 1.0.0 DESCRIPTION "GLib project")
enable_testing()

#add_subdirectory(GLib) # is dep of Tests, try https://stackoverflow.com/questions/33443164/cmake-share-library-with-multiple-executables
add_subdirectory(Tests)

if(WIN32)
	add_subdirectory(Coverage)
	add_subdirectory(TestApp)
endif(WIN32)