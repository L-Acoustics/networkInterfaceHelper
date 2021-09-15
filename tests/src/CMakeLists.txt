# Network Interface Helper tests

# gtest include directories
include_directories(${gtest_SOURCE_DIR}/include)

# gtest link directories
link_directories(${gtest_BINARY_DIR}/src)

# Set gtest library
set(ADD_LINK_LIBRARIES gtest)

### Unit Tests
set(TESTS_SOURCE
	main.cpp
	networkInterfaceHelper_tests.cpp
	ipAddress_tests.cpp
)
list(APPEND ADD_LINK_LIBRARIES la_networkInterfaceHelper_static)

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${TESTS_SOURCE})

# Define target
add_executable(Tests ${TESTS_SOURCE})

# Setup common options
cu_setup_executable_options(Tests)

# Additional private include directory
target_include_directories(Tests PRIVATE "${CU_ROOT_DIR}/src")

# Set IDE folder
set_target_properties(Tests PROPERTIES FOLDER "Tests")

# Link with required libraries
target_link_libraries(Tests PRIVATE ${LINK_LIBRARIES} ${ADD_LINK_LIBRARIES})

# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(Tests ${INSTALL_TEST_FLAG} ${SIGN_FLAG})
