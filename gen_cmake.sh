#!/usr/bin/env bash
# Useful script to generate project files using cmake

# Override default cmake options
cmake_opt="-DBUILD_NIH_EXAMPLES=TRUE -DBUILD_NIH_TESTS=TRUE -DBUILD_C_BINDINGS=TRUE -DBUILD_NIH_SWIG=FALSE -DINSTALL_NIH_EXAMPLES=TRUE -DINSTALL_NIH_TESTS=TRUE -DINSTALL_NIH_LIB_STATIC=TRUE -DINSTALL_NIH_LIB_C=TRUE -DINSTALL_NIH_LIB_SWIG=TRUE -DINSTALL_NIH_HEADERS=TRUE -DENABLE_CODE_SIGNING=FALSE"

############################ DO NOT MODIFY AFTER THAT LINE #############

# Get absolute folder for this script
selfFolderPath="`cd "${BASH_SOURCE[0]%/*}"; pwd -P`/" # Command to get the absolute path

# Include default values
if [ -f "${selfFolderPath}.defaults.sh" ]; then
	. "${selfFolderPath}.defaults.sh"
fi

gen_c=0
gen_csharp=0

function extend_gc_fnc_help()
{
	echo " -build-c -> Build the C bindings library."
	echo " -build-csharp -> Build the C# bindings library."
}

function extend_gc_fnc_unhandled_arg()
{
	case "$1" in
		-build-c)
			gen_c=1
			return 1
			;;
		-build-csharp)
			gen_csharp=1
			return 1
			;;
	esac
	return 0
}

# function extend_gc_fnc_postparse()
# {
# }

function extend_gc_fnc_precmake()
{
	if [ $gen_c -eq 1 ]; then
		add_cmake_opt+=("-DBUILD_C_BINDINGS=TRUE")
		add_cmake_opt+=("-DINSTALL_NIH_LIB_C=TRUE")
	fi
	if [ $gen_csharp -eq 1 ]; then
		add_cmake_opt+=("-DBUILD_NIH_SWIG=TRUE")
		add_cmake_opt+=("-DINSTALL_NIH_LIB_SWIG=TRUE")
		add_cmake_opt+=("-DNIH_SWIG_LANGUAGES=csharp")
	fi
}

# execute gen_cmake script from bashUtils
. "${selfFolderPath}scripts/bashUtils/gen_cmake.sh"
