project (ModeChecker)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (ModeChecker "src/mode_checker.c")
add_dependencies (ModeChecker traffic_light_idl_files traffic_light_mode_checker_edl_files)