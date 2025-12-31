# Lua example

# Copy the Lua script to the build directory
configure_file(networkInterfacesEnumerator.lua ${CMAKE_CURRENT_BINARY_DIR}/networkInterfacesEnumerator.lua COPYONLY)

# Deploy Lua SWIG bindings and its dependencies to the binary directory
cu_deploy_lua_bindings(networkInterfacesEnumeratorLua la_networkInterfaceHelper-lua la_networkInterfaceHelper)
