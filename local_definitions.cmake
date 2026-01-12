# Override the default warning flags.
# Disable GCC -O3 (aggressive static analysis) false positive when checking bounds for char sized (considered string) arrays for the auto-generated code from SWIG
cu_set_warning_flags(TARGETS la_networkInterfaceHelper-csharp COMPILER GCC PRIVATE -Wno-stringop-overflow)
