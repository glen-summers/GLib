
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
	set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
		#message("static link ${CompilerFlag} ${${CompilerFlag}}")
	endforeach()

	string(REPLACE "/W3" "/W4 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	#message("warnings CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")
else(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")
endif (WIN32)

function(AddStdLinkage target)
if (UNIX)
	target_link_libraries(${target} stdc++fs)
endif (UNIX)
endfunction(AddStdLinkage)

function(R_SEARCH_ABOVE search_path root_item sub_item return_value)
while(true)
	if (EXISTS "${search_path}/${root_item}/${sub_item}")
		break()
	else()
		get_filename_component(parent_dir ${search_path} DIRECTORY)
		if (${parent_dir} STREQUAL ${search_path})
			break()
		endif()
		set(search_path ${parent_dir})
	endif()
endwhile()
set(${return_value} ${search_path}/${root_item} PARENT_SCOPE)
endfunction(R_SEARCH_ABOVE)
