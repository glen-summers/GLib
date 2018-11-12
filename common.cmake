
macro(SetCpp17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
endmacro()

if (WIN32)
# force static linkage for msvc
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	set(CompilerFlags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
	foreach(CompilerFlag ${CompilerFlags})
	  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
endif (WIN32)

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