project (Exchange)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (Exchange "src/exchange.c")
add_dependencies (Exchange traffic_light_idl_files traffic_light_exchange_edl_files)