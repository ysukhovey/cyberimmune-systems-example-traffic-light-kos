project (MicroJSON)

# Set compile flags.
project_header_default ("STANDARD_GNU_11:YES" "STRICT_WARNINGS:NO")

add_library (MicroJSON "src/mjson.c")
