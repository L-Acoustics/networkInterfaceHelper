# Library CMake file

cu_setup_project(la_networkInterfaceHelper ${LA_NIH_VERSION} "Network Interface Helper Library")

# Configure files based on CMakeLists.txt version number
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

set(ADD_LINK_LIBS "")
set(ADD_PUBLIC_COMPILE_OPTIONS "")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Add pthread library on linux
	list(APPEND ADD_LINK_LIBS "-lpthread")
endif()

# Public header files
set (PUBLIC_HEADER_FILES
	${CU_ROOT_DIR}/include/la/networkInterfaceHelper/networkInterfaceHelper.hpp
)

# Common files
set (HEADER_FILES_COMMON
	networkInterfaceHelper_common.hpp
	${CMAKE_CURRENT_BINARY_DIR}/config.hpp
)

set (SOURCE_FILES_COMMON
	libraryInfo.cpp
	networkInterfaceHelper_common.cpp
	ipAddress.cpp
	ipAddressInfo.cpp
)

# OS-dependent files
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set (SOURCE_FILES_OS_DEPENDENT
		networkInterfaceHelper_win32.cpp
		helper_win32.cpp
	)
	list(APPEND PUBLIC_HEADER_FILES "${CU_ROOT_DIR}/include/la/networkInterfaceHelper/windowsHelper.hpp")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set (SOURCE_FILES_OS_DEPENDENT
		networkInterfaceHelper_unix.cpp
	)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set (SOURCE_FILES_OS_DEPENDENT
		networkInterfaceHelper_mac.mm
	)
	list(APPEND ADD_LINK_LIBS "-framework Foundation -framework SystemConfiguration -framework IOKit")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set (SOURCE_FILES_OS_DEPENDENT
		networkInterfaceHelper_ios.mm
	)
	list(APPEND ADD_LINK_LIBS "-framework Foundation -framework SystemConfiguration")
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
	set (SOURCE_FILES_OS_DEPENDENT
		# Use the unix version for now, only interface type seems to be missing 
		networkInterfaceHelper_unix.cpp
	)
else()
	message(FATAL_ERROR "Network helper undefined for system: ${CMAKE_SYSTEM_NAME}")
endif()

# Group sources
source_group("Header Files\\Public" FILES ${PUBLIC_HEADER_FILES})
source_group("Header Files\\Internal" FILES ${HEADER_FILES_COMMON})
source_group("Source Files\\Common" FILES ${SOURCE_FILES_COMMON})
source_group("Source Files\\OsDependent" FILES ${SOURCE_FILES_OS_DEPENDENT})

set (LIB_HEADER_FILES ${HEADER_FILES_COMMON} ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON} ${SOURCE_FILES_OS_DEPENDENT})

add_library(${PROJECT_NAME}_static STATIC ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES})
if(ADD_PUBLIC_COMPILE_OPTIONS)
	target_compile_options(${PROJECT_NAME}_static PUBLIC ${ADD_PUBLIC_COMPILE_OPTIONS})
endif()
if(ADD_LINK_LIBS)
	target_link_libraries(${PROJECT_NAME}_static PUBLIC ${ADD_LINK_LIBS})
endif()

# Setup common library options
cu_setup_library_options(${PROJECT_NAME}_static)

# Additional include directory (only for build interface)
target_include_directories(${PROJECT_NAME}_static PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>) # Include root source directory for access to private headers

# Setup install (and signing)
cu_setup_deploy_library(${PROJECT_NAME}_static ${INSTALL_STATIC_FLAG})

# Set headers installation rules
if(INSTALL_NIH_HEADERS)
	cu_setup_headers_install_rules("${PUBLIC_HEADER_FILES}" "${CU_ROOT_DIR}/include")
endif()

# C Bindings library
if(BUILD_C_BINDINGS)
	add_subdirectory(bindings/c)
endif()

# SWIG Wrapper
if(BUILD_NIH_SWIG)
	include(${CU_BASE_DIR}/helpers/SetupSwig.cmake REQUIRED)

	set(TARGET_PREFIX ${PROJECT_NAME})
	set(SWIG_INTERFACE_FILE "${CU_ROOT_DIR}/include/la/networkInterfaceHelper/networkInterfaceHelper.i")
	cu_setup_swig_target(VERSION 4.2 TARGET_NAME ${PROJECT_NAME}_static INTERFACE_FILE "${SWIG_INTERFACE_FILE}" LANGUAGES ${NIH_SWIG_LANGUAGES} SWIG_TARGET_PREFIX ${TARGET_PREFIX} REQUIRED INSTALL_SUPPORT_FILES)

	# Post process generated targets
	foreach(SWIG_LANG ${NIH_SWIG_LANGUAGES})
		set(SWIG_TARGET_NAME ${TARGET_PREFIX}-${SWIG_LANG})

		# Add all public header files as dependency, so that wrapper files are re-generated whenever they change
		set_property(TARGET ${SWIG_TARGET_NAME} PROPERTY SWIG_DEPENDS ${PUBLIC_HEADER_FILES})

		# Setup common options like any other project
		cu_setup_library_options(${SWIG_TARGET_NAME} ALIAS_NAME ${SWIG_LANG})

		if(${SWIG_LANG} STREQUAL "lua")
			target_link_libraries(${SWIG_TARGET_NAME} PRIVATE liblua)
		elseif(${SWIG_LANG} STREQUAL "python")
			find_package(Python3 COMPONENTS Development REQUIRED)
			target_link_libraries(${SWIG_TARGET_NAME} PRIVATE Python3::Python)
		endif()

		# Setup install (and signing)
		cu_setup_deploy_library(${SWIG_TARGET_NAME} ${INSTALL_SWIG_FLAG} ${SIGN_FLAG})

	endforeach()
endif()
