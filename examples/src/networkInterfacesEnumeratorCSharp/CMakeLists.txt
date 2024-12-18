# CSharp example

include(${CU_BASE_DIR}/helpers/GenerateCSharpTarget.cmake REQUIRED)

set(SWIG_CSHARP_TARGET_NAME "la_networkInterfaceHelper-csharp")
set(CU_CSHARP_ADDITIONAL_COMPILE_ITEMS "<Compile Include=\"$<TARGET_FILE_DIR:${SWIG_CSHARP_TARGET_NAME}>/SWIG_${SWIG_CSHARP_TARGET_NAME}/csharp.files/**/*.cs\" />")
set(CU_CSHARP_ADDITIONAL_CONTENT_ITEMS "<Content Include=\"$<TARGET_FILE:${SWIG_CSHARP_TARGET_NAME}>\" Link=\"${CMAKE_SHARED_LIBRARY_PREFIX}$<TARGET_FILE_BASE_NAME:${SWIG_CSHARP_TARGET_NAME}>${CMAKE_SHARED_LIBRARY_SUFFIX}\"><CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory></Content>")

cu_generate_csharp_target(TARGET_NAME "NetworkInterfacesEnumerator" ADDITIONAL_DEPENDENCIES "${SWIG_CSHARP_TARGET_NAME}")
