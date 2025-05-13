# LA Network Interface Helper Library Changelog
All notable changes to the LA Network Interface Helper Library will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Fixed
- On macOS, correctly retrieve the user defined name of the interface.
- On macOS, correctly detect plugged/unplugged adapters (partially broken since macOS 11).
- IPv6 from string conversion not working if shortened at the end (e.g. 'acbd::').

## [1.2.5] - 2025-01-27
### Fixed
- Missing SWIG type for IPAddressPackedV6.
- Missing SWIG `director` attribute for `DefaultedObserver`.

## [1.2.4] - 2024-12-20

## [1.2.3] - 2024-12-11
### Fixed
- Linux compilation.

## [1.2.2] - 2024-12-11
### Changed
- Improved C# NuGet package.

## [1.2.1] - 2024-12-05
### Changed
- Improved C# NuGet package.

## [1.2.0] - 2024-12-03
### Added
- C# example.
- IPv6 support.
- NuGet support.
- CMakePresets support.

### Fixed
- `size_t` size for C# bindings (based on arch).

## [1.1.1] - 2024-06-17
### Added
- Docker support.

### Fixed
- Various toolchain issues.

## [1.1.0] - 2023-11-06
### Added
- C-bindings for the library.
- SWIG C# bindings for the library.
- `DefaultedObserved` class to allow for easier observation of network interface changes.

### Fixed
- Various bug fixes on windows.

## [1.0.0] - 2021-09-15
### Added
- Initial release of the library.
