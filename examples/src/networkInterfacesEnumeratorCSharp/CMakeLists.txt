# CSharp example

include(${CU_BASE_DIR}/helpers/GenerateCSharpTarget.cmake REQUIRED)

set(CSHARP_DEPENDENCIES "la_networkInterfaceHelper-csharp")

foreach(_DEP ${CSHARP_DEPENDENCIES})
	cu_generate_csharp_target_add_csharp_dependency(${_DEP})
endforeach()

cu_generate_csharp_target(TARGET_NAME "NetworkInterfacesEnumerator" ADDITIONAL_DEPENDENCIES ${CSHARP_DEPENDENCIES})
