# LA Network Interface Helper Library
Copyright (C) 2016-2026, L-Acoustics

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
- Visual Studio 2022 v17.14 or greater
- GitBash, cygwin or WSL

### macOS
- macOS 11
- Xcode 26

### Linux
- C++17 compliant compiler (minimum recommended g++ 11.2.0)
- Make

## Platform notes

### macOS 27 and later
Starting with macOS 27, the kernel redacts the link-layer (MAC) address of all network interfaces for non-root processes: *getifaddrs*, *sysctl* and *ioctl* all return *02:00:00:00:00:00* (the same privacy hardening that exists on iOS). This is a deliberate change by Apple, not a bug. The library transparently falls back to the SystemConfiguration framework (the same source used by the *networksetup* command line tool) to retrieve the hardware address of physical interfaces, so no change is required in applications using this library.

Virtual interfaces that are not backed by a hardware controller (eg. *bridge0*) do not have a hardware address in SystemConfiguration and are reported with the redacted value.

If your application needs the real link-layer address from the kernel APIs directly (for example to send raw Ethernet frames through BPF), it must hold the *com.apple.developer.networking.topology-observation* entitlement (available to all Apple developers by enabling the *Network Topology Observation* capability in Xcode 27). This is a restricted entitlement that must be authorized by a provisioning profile, which means command line tools have to be embedded in an app-like bundle (see Apple's *Signing a daemon with a restricted entitlement* documentation).

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
