# Network Interface Helper examples

# Network interfaces enumerator
add_executable(NetworkInterfacesEnumerator networkInterfacesEnumerator.cpp)
set_target_properties(NetworkInterfacesEnumerator PROPERTIES FOLDER "Examples")
# Setup common options
cu_setup_executable_options(NetworkInterfacesEnumerator)
# Using NIH library
target_link_libraries(NetworkInterfacesEnumerator PRIVATE la_networkInterfaceHelper_static)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(NetworkInterfacesEnumerator ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

# Network interfaces monitor
add_executable(NetworkInterfacesMonitor networkInterfacesMonitor.cpp)
set_target_properties(NetworkInterfacesMonitor PROPERTIES FOLDER "Examples")
# Setup common options
cu_setup_executable_options(NetworkInterfacesMonitor)
# Using NIH library
target_link_libraries(NetworkInterfacesMonitor PRIVATE la_networkInterfaceHelper_static)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(NetworkInterfacesMonitor ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

if(BUILD_C_BINDINGS)
	# Network interfaces enumerator using C bindings
	add_executable(NetworkInterfacesEnumeratorC networkInterfacesEnumerator.c)
	set_target_properties(NetworkInterfacesEnumeratorC PROPERTIES FOLDER "Examples/Bindings/C")
	# Setup common options
	cu_setup_executable_options(NetworkInterfacesEnumeratorC)
	# Using NIH library
	target_link_libraries(NetworkInterfacesEnumeratorC PRIVATE la_networkInterfaceHelper_c_static)
	# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
	cu_setup_deploy_runtime(NetworkInterfacesEnumeratorC ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})
endif()
