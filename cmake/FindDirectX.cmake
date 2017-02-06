# Find DirectX 11
include (CheckIncludeFileCXX)
# Find a header in the Windows SDK
macro (find_winsdk_header var_name header)
	# Windows SDK
	set (include_dir_var "DirectX_${var_name}_INCLUDE_DIR")
	set (include_found_var "DirectX_${var_name}_INCLUDE_FOUND")
	#check_include_file_cxx (${header} ${include_found_var})
	set (${include_dir_var} ${header})
	mark_as_advanced (${include_found_var})
endmacro ()

# Find a library in the Windows SDK
macro (find_winsdk_library var_name library)
	# XXX: We currently just assume the library exists
	set (library_var "DirectX_${var_name}_LIBRARY")
	set (${library_var} ${library})
	mark_as_advanced (${library_var})
	MESSAGE( STATUS "!!!!!!DX_LIB = :         " ${library_var} )
endmacro ()

# Combine header and library variables into an API found variable
macro (find_combined var_name inc_var_name lib_var_name)
	if (DirectX_${inc_var_name}_INCLUDE_FOUND AND DirectX_${lib_var_name}_LIBRARY)
		set (DirectX_${var_name}_FOUND 1)
		find_package_message (${var_name} "Found ${var_name} API" "[${DirectX_${lib_var_name}_LIBRARY}][${DirectX_${inc_var_name}_INCLUDE_DIR}]")
	endif ()
endmacro ()

find_winsdk_header  (D3D11   d3d11.h)
find_winsdk_library (D3D11   d3d11)
find_winsdk_library (D3DCompiler D3DCompiler)