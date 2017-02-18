# Add resource depency to copy changes to resource files
set(RESOURCE_DEPENDENCY "${CMAKE_CURRENT_BINARY_DIR}/Resource.h")
 
add_custom_command(OUTPUT ${RESOURCE_DEPENDENCY}
				   COMMAND ${CMAKE_COMMAND} -E copy_directory ${BASE_DIR}/resources/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/
				   COMMENT "Copying Resource Files.")

add_custom_target(ResourceBuild DEPENDS ${RESOURCE_DEPENDENCY})				   
				   
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_SOURCE_DIR}/resources/*.*")
				   
macro (create_example_executable name opengl directx)

set (example_name ${name})
set (example_full_name Example-${name})

file(GLOB EXAMPLE_SOURCES
	"${CMAKE_CURRENT_LIST_DIR}/${example_name}/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/${example_name}/*.h")

source_group("src" FILES ${EXAMPLE_SOURCES})

# Add OpenGL
if(${opengl})	
	# GLEW
	file(GLOB GLEW_SOURCES
		"${DEPENDENCIES_DIR}/glew/*.c")
	source_group("lib\\glew" FILES ${GLEW_SOURCES})
	
	# Create example executable project
	add_executable(${example_full_name} ${EXAMPLE_SOURCES} ${GLEW_SOURCES})
	
	# GLEW
	target_include_directories(${example_full_name} PUBLIC ${DEPENDENCIES_DIR}glew/include/)
	
	# GLFW
	target_include_directories(${example_full_name} PUBLIC ${DEPENDENCIES_DIR}glfw/include/)
	target_link_libraries(${example_full_name} ${DEPENDENCIES_DIR}glfw/lib/glfw3.lib)
	
	# Link OpenGL
	target_link_libraries(${example_full_name} ${OPENGL_LIBRARIES})
	
elseif(${directx})
	# Create example executable project
	add_executable(${example_full_name} ${EXAMPLE_SOURCES})
else()
	# Create example executable project
	add_executable(${example_full_name} ${EXAMPLE_SOURCES})
endif(${opengl})

# Add Basic Model Static Lib
target_include_directories(${example_full_name} PUBLIC ${BMDL_INCLUDE})
add_dependencies(${example_full_name} BasicModel)
add_dependencies(${example_full_name} ResourceBuild)
target_link_libraries(${demo_full_name} BasicModel)

# Link OpenGL
target_link_libraries(${example_full_name} ${OPENGL_LIBRARIES})

set_target_properties(${example_full_name} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${example_full_name} PROPERTIES FOLDER "Examples")

if(WIN32)
	set_target_properties(${example_full_name} PROPERTIES COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
endif(WIN32)

if(MSVC)
	
	# Ensure project output directory doesn't change for different configurations
	set_target_properties( ${example_full_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIR} )
	foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
		set_target_properties( ${example_full_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${RUNTIME_OUTPUT_DIR} )
		set_target_properties( ${example_full_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${ARCHIVE_OUTPUT_DIR} )
		set_target_properties( ${example_full_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${ARCHIVE_OUTPUT_DIR} )
		message("CONFIG_SET = " ${OUTPUTCONFIG})
	endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
	
	# Add a user file to set the working directory for debugging in Visual Studio
	configure_file(${CMAKE_SOURCE_DIR}/cmake/WorkingDirTemplate.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${example_full_name}.vcxproj.user @ONLY)

endif(MSVC)

endmacro ()

# Example : DirectX11
create_example_executable(DirectX11 FALSE TRUE)

# Example : OpenGL3
create_example_executable(OpenGL3 TRUE FALSE)

# Example : Simple
create_example_executable(Simple FALSE FALSE)

