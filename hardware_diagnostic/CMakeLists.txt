project (HardwareDiagnostic)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (HardwareDiagnostic "src/hardware_diagnostic.c")
add_dependencies (HardwareDiagnostic traffic_light_idl_files traffic_light_hardware_diagnostic_edl_files)