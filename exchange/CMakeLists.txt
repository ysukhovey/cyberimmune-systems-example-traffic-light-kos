project (Exchange)

# Tools for using NK parser.
include (platform/nk)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_executable (Exchange "src/exchange.c")
add_dependencies (Exchange traffic_light_idl_files traffic_light_exchange_edl_files)
add_dependencies (Exchange MicroJSON)

target_link_libraries (Exchange
        ${${PROJECT_NAME}_LIBS}
        rump::dhcpcd_client
        ${vfs_CLIENT_LIB}
        MicroJSON
        pthread)

# We do not need the default VFS entity which comes from ${vfs_CLIENT_LIB}.
set_target_properties (Exchange PROPERTIES ${vfs_ENTITY}_REPLACEMENT "")
set_target_properties (Exchange PROPERTIES LINK_FLAGS ${VULN_LDFLAGS})