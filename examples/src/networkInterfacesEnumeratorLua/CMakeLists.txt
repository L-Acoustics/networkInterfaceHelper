# Lua example

# Copy the Lua script to the build directory
configure_file(networkInterfacesEnumerator.lua ${CMAKE_CURRENT_BINARY_DIR}/networkInterfacesEnumerator.lua COPYONLY)

# Custom target to copy the SWIG Lua library to the build directory so the script can find it
add_custom_target(copy_lua_library ALL
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:la_networkInterfaceHelper-lua> ${CMAKE_CURRENT_BINARY_DIR}/la_networkInterfaceHelper${CMAKE_SHARED_MODULE_SUFFIX}
    DEPENDS la_networkInterfaceHelper-lua
    COMMENT "Copying Lua SWIG library to example directory"
)
