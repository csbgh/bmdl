# BMDL Base Source
file(GLOB BMDL_SOURCES
	"${CMAKE_CURRENT_LIST_DIR}/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/*.h")
source_group("src" FILES ${BMDL_SOURCES})

file(GLOB BMDL_FORMATS_SOURCES
	"${CMAKE_CURRENT_LIST_DIR}/formats/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/formats/*.h")
source_group("formats" FILES ${BMDL_FORMATS_SOURCES})
	
file(GLOB BMDL_EXTENSIONS_SOURCES
	"${CMAKE_CURRENT_LIST_DIR}/extensions/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/extensions/*.h")
source_group("extensions" FILES ${BMDL_EXTENSIONS_SOURCES})
	
add_library(BasicModel STATIC
			${BMDL_SOURCES} 
			${BMDL_FORMATS_SOURCES}
			${BMDL_EXTENSIONS_SOURCES} )

set_target_properties(BasicModel PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET BasicModel PROPERTY FOLDER "Basic Model")

if(WIN32)
	set_property(TARGET BasicModel PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS GLEW_STATIC)
endif(WIN32)

target_include_directories(BasicModel PUBLIC  ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(BasicModel PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/formats/)
target_include_directories(BasicModel PUBLIC  ${CMAKE_CURRENT_LIST_DIR}/extensions)
