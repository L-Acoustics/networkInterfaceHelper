#!/usr/bin/env bash
# Useful script to publish a C# NuGet package

libName=la_networkInterfaceHelper
configType=Release
add_cmake_opt=("-DBUILD_NIH_SWIG=TRUE" "-DNIH_SWIG_LANGUAGES=csharp")

############################ DO NOT MODIFY AFTER THAT LINE #############

# Get absolute folder for this script
selfFolderPath="`cd "${BASH_SOURCE[0]%/*}"; pwd -P`/" # Command to get the absolute path

# execute publish_nuget script from bashUtils
. "${selfFolderPath}scripts/bashUtils/publish_nuget.sh" -l $libName -c $configType $@ -- ${add_cmake_opt[@]}
