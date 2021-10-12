# C Bindings CMake file

cu_setup_project(la_networkInterfaceHelper_c ${LA_NIH_VERSION} "Network Interface Helper C Bindings Library")

# Public header files
set (PUBLIC_HEADER_FILES
	${CU_ROOT_DIR}/include/la/networkInterfaceHelper/networkInterfaceHelper.h
	${CU_ROOT_DIR}/include/la/networkInterfaceHelper/internals/exports.h
)

# Common files
set (SOURCE_FILES_COMMON
	networkInterfaceHelper_c.cpp
)

# Group sources
source_group("Header Files\\Public" FILES ${PUBLIC_HEADER_FILES})
source_group("Source Files" FILES ${SOURCE_FILES_COMMON})

set (LIB_HEADER_FILES ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON})

add_library(${PROJECT_NAME}_static STATIC ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES})

# Setup common library options
cu_setup_library_options(${PROJECT_NAME}_static)

# Using NIH library
target_link_libraries(${PROJECT_NAME}_static PRIVATE la_networkInterfaceHelper_static)

# Additional include directory (only for build interface)
target_include_directories(${PROJECT_NAME}_static PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>) # Include root source directory for access to private headers

# Setup install (and signing)
cu_setup_deploy_library(${PROJECT_NAME}_static INSTALL ${SIGN_FLAG})

# Set headers installation rules
cu_setup_headers_install_rules("${PUBLIC_HEADER_FILES}" "${CU_ROOT_DIR}/include")
