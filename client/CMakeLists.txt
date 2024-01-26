project (client)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (Client "src/client.c")
add_dependencies (Client echo_idl_files echo_client_edl_files)

#set_target_properties (Client PROPERTIES LINK_FLAGS "-no-pie -Ttext 0x00800000")
