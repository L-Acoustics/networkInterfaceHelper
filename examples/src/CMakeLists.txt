# Network Interface Helper examples

# Network interfaces enumerator
add_executable(NetworkInterfacesEnumerator networkInterfacesEnumerator.cpp)
set_target_properties(NetworkInterfacesEnumerator PROPERTIES FOLDER "Examples")
# Using NIH library
target_link_libraries(NetworkInterfacesEnumerator PRIVATE la_networkInterfaceHelper_static)
# Setup common options
cu_setup_executable_options(NetworkInterfacesEnumerator)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(NetworkInterfacesEnumerator ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

# Network interfaces monitor
add_executable(NetworkInterfacesMonitor networkInterfacesMonitor.cpp)
set_target_properties(NetworkInterfacesMonitor PROPERTIES FOLDER "Examples")
# Using NIH library
target_link_libraries(NetworkInterfacesMonitor PRIVATE la_networkInterfaceHelper_static)
# Setup common options
cu_setup_executable_options(NetworkInterfacesMonitor)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(NetworkInterfacesMonitor ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})
