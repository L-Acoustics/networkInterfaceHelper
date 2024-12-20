# L-Acoustics Network Interface Helper Library

# Minimum CMake version
cmake_minimum_required(VERSION 3.22)

############ Versioning

# Using Semantic Versioning (http://semver.org/spec/v2.0.0.html)
# This variable defines the project version number.
# It should have 3 digits (x.y.z) on main branch, and 4 digits (x.y.z.w) on dev and task branches. The last digit being used as beta label.
# As soon as an API change occurs in dev or task branch, update the version number by following SemVer rules (so the API breaking changes are not lost/forgotten).
# Example: 1.5.0 is the upcoming release version (main branch) of current 1.5.0.3 devel version (dev/task) which will be labelled 1.5.0-beta3
set(LA_NIH_VERSION 1.2.4)

############ Override from command line "CMake -D<OPTION>=TRUE/FALSE/0/1/ON/OFF"

# Build options
option(BUILD_NIH_EXAMPLES "Build examples." FALSE)
option(BUILD_NIH_TESTS "Build unit tests." FALSE)
option(BUILD_C_BINDINGS "Build C Bindings." FALSE)
option(BUILD_NIH_SWIG "Build SWIG Wrapper." FALSE)
# Install options
option(INSTALL_NIH_EXAMPLES "Install examples." FALSE)
option(INSTALL_NIH_TESTS "Install unit tests." FALSE)
option(INSTALL_NIH_LIB_STATIC "Install static library." FALSE)
option(INSTALL_NIH_LIB_C "Install C Bindings libraries." FALSE)
option(INSTALL_NIH_LIB_SWIG "Install swig libraries." FALSE)
option(INSTALL_NIH_HEADERS "Install headers." FALSE)
# Signing options
option(ENABLE_CODE_SIGNING "Enable binaries signing." FALSE)

############ Override from command line "CMake -D<PARAM>=<VALUE>"

# Configuration options
if(NOT NIH_SWIG_LANGUAGES)
	set(NIH_SWIG_LANGUAGES "csharp" CACHE STRING "Set the languages to compile as SWIG wrappers." FORCE)
endif()

############ CMake configuration

project(LA_networkInterfaceHelper)

# Using c++17 standard for all targets
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Define some product/company specific values
set(CU_PROJECT_FULL_NAME "Network InterfaceHelper Library")
set(CU_COMPANY_NAME "L-Acoustics")
set(CU_COMPANY_URL "http://www.l-acoustics.com")
set(CU_COMPANY_DOMAIN "com")
set(CU_PROJECT_PRODUCTDESCRIPTION "Lightweight open source library for enumerating Network Interfaces and monitoring any state change.")
set(CU_PROJECT_CONTACT "software@l-acoustics.com")
set(CU_PROJECT_STARTING_YEAR "2016")
set(CU_COPYRIGHT_HOLDER "L-Acoustics and its contributors")

# Enable cmake folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Configure installation path: we override the default installation path.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "./Install" CACHE PATH "default install path" FORCE)
endif()

# Define base folder for cmakeUtils scripts
set(CU_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake/cmakeUtils")

# Include project macros
include(${CU_BASE_DIR}/ProjectMacros.cmake)

# Include common properties overrides
include(${CU_BASE_DIR}/ProjectProperties.cmake)

# Immediately setup variables related to the project version
cu_setup_project_version_variables(${LA_NIH_VERSION})

# Set minimum OSX/iOS version
cu_setup_apple_minimum_versions(MACOS 10.9 IOS 11)

############ Define some variables

if(${INSTALL_NIH_LIB_STATIC})
	set(INSTALL_STATIC_FLAG "INSTALL")
endif()
if(${INSTALL_NIH_LIB_C})
	set(INSTALL_C_FLAG "INSTALL")
endif()
if(${INSTALL_NIH_LIB_SWIG})
	set(INSTALL_SWIG_FLAG "INSTALL")
endif()
if(${INSTALL_NIH_EXAMPLES})
	set(INSTALL_EXAMPLE_FLAG "INSTALL")
endif()
if(${INSTALL_NIH_TESTS})
	set(INSTALL_TEST_FLAG "INSTALL")
endif()
if(${ENABLE_CODE_SIGNING})
	set(SIGN_FLAG "SIGN")
endif()

############ Add projects

# Add main library
message(STATUS "Building Network Interface Helper Library")
add_subdirectory(src)

# Add examples
if(BUILD_NIH_EXAMPLES)
	message(STATUS "Building examples")
	add_subdirectory(examples)
	# Force SimpleController as visual default target
	set_directory_properties(PROPERTIES VS_STARTUP_PROJECT NetworkInterfacesEnumerator)
endif()

# Add tests
if(BUILD_NIH_TESTS)
	message(STATUS "Building unit tests")
	# Setup gtest options
	set(gtest_force_shared_crt ON CACHE BOOL "For shared crt for gtest" FORCE)
	set(BUILD_GMOCK OFF CACHE BOOL "Don't build the googlemock subproject" FORCE)
	set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" FORCE)
	# Include gtest framework
	add_subdirectory(externals/3rdparty/gtest)
	# Include our unit tests
	add_subdirectory(tests)
endif()

############ Compiler compatibility

if(WIN32)
	if(${MSVC_VERSION} STRLESS 1923)
		message(FATAL_ERROR "Microsoft Visual C++ v16.3 required (MSVC_VERSION >= 1923), you have ${MSVC_VERSION}")
	endif()
endif()

# Configure a few things that has to be done last
cu_finalize(NO_VSCODE_WORKSPACE)
