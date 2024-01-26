project (server)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (Server "src/server.c")
add_dependencies (Server echo_server_edl_files)

#set_target_properties (Server PROPERTIES LINK_FLAGS "-no-pie -Ttext 0x00c00000")
