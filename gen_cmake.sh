#!/usr/bin/env bash
# Useful script to generate project files using cmake

cmake_opt="-DBUILD_NIH_EXAMPLES=TRUE -DBUILD_NIH_TESTS=TRUE -DBUILD_C_BINDINGS=TRUE -DBUILD_NIH_SWIG=FALSE -DINSTALL_NIH_EXAMPLES=TRUE -DINSTALL_NIH_TESTS=TRUE -DINSTALL_NIH_LIB_STATIC=TRUE -DINSTALL_NIH_LIB_C=TRUE -DINSTALL_NIH_LIB_SWIG=TRUE -DINSTALL_NIH_HEADERS=TRUE -DENABLE_CODE_SIGNING=FALSE"

############################ DO NOT MODIFY AFTER THAT LINE #############

# Get absolute folder for this script
selfFolderPath="`cd "${BASH_SOURCE[0]%/*}"; pwd -P`/" # Command to get the absolute path

# execute gen_cmake script from bashUtils
. "${selfFolderPath}scripts/bashUtils/gen_cmake.sh"
