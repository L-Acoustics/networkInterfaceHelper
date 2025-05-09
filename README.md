[![Linux build & test](https://github.com/L-Acoustics/networkInterfaceHelper/actions/workflows/linux.yml/badge.svg)](https://github.com/L-Acoustics/networkInterfaceHelper/actions/workflows/linux.yml)
[![Windows](https://github.com/L-Acoustics/networkInterfaceHelper/actions/workflows/windows.yml/badge.svg)](https://github.com/L-Acoustics/networkInterfaceHelper/actions/workflows/windows.yml)
[![Macos](https://github.com/L-Acoustics/networkInterfaceHelper/actions/workflows/macos.yml/badge.svg)](https://github.com/L-Acoustics/networkInterfaceHelper/actions/workflows/macos.yml)
# LA Network Interface Helper Library
Copyright (C) 2016-2025, L-Acoustics

## What is LA_networkInterfaceHelper
LA_networkInterfaceHelper is a lightweight open source library for enumerating Network Interfaces and monitoring any state change.

This library are written in pure C++17. It can be compiled on Windows, Linux and macOS, using standard development tools (procedure below). The library can target the following platforms: Windows, Linux, macOS and iOS.

Unit tests and sample programs are also provided, and a [SWIG](http://www.swig.org) interface file is proposed for easy integration with other languages.

We use GitHub issues for tracking requests and bugs.

## Licensing
This software is licensed under the BSD 3-clause License (see [LICENSE](LICENSE)).

## Minimum requirements for compilation

### All platforms
- CMake 3.22

### Windows
- Windows 10
- Visual Studio 2022 v17.6 or greater
- GitBash, cygwin or WSL

### macOS
- macOS 10.13
- Xcode 15

### Linux
- C++17 compliant compiler (minimum recommended g++ 11.2.0)
- Make

### Optional dependencies:
* [Google's C++ test framework](https://github.com/google/googletest) to build unit tests

## Compilation

### All platforms
- Clone this repository
- Update submodules: *git submodule update --init*

### Windows
- Using the provided bash script (*gen_cmake.sh*):
  * Run the script with whatever optional parameters required (run *gen_cmake.sh -h* to display the help)
  * Go into the generated output folder
  * Open the generated Visual Studio solution *LA_networkInterfaceHelper.sln*
  * Compile everything from Visual Studio
- Manually issuing a CMake command:
  * Run a proper CMake command to generate a Visual Studio solution (or any other CMake generator matching your build toolchain)
  * Open the generated Visual Studio solution (or your other CMake generated files)
  * Compile everything from Visual Studio (or compile using your toolchain)
 
### macOS
- Using the provided bash script (*gen_cmake.sh*):
  * Run the script with whatever optional parameters required (run *gen_cmake.sh -h* to display the help)
  * Go into the generated output folder
  * Open the generated Xcode solution *LA_networkInterfaceHelper.xcodeproj*
  * Compile everything from Xcode
- Use the *-ios* command switch when invoking *gen_cmake.sh* if you want to target the iOS platform
- Manually issuing a CMake command:
  * Run a proper CMake command to generate a Xcode solution (or any other CMake generator matching your build toolchain)
  * Open the generated Xcode solution (or your other CMake generated files)
  * Compile everything from Xcode (or compile using your toolchain)

### Linux
- Using the provided bash script (*gen_cmake.sh*):
  * Run the script with either *-debug* or *-release* and whatever optional parameters required (run *gen_cmake.sh -h* to display the help)
  * Go into the generated output folder
  * Run *make* to compile everything
- Manually issuing a CMake command:
  * Run a proper CMake command to generate Unix Makefiles (or any other CMake generator matching your build toolchain)
  * Go into the folder where the Unix Makefiles have been generated
  * Run *make* to compile everything (or compile using your toolchain)

## Contributing code

[Please read this file](CONTRIBUTING.md)

## Trademark legal notice
All product names, logos, brands and trademarks are property of their respective owners. All company, product and service names used in this library are for identification purposes only. Use of these names, logos, and brands does not imply endorsement.
