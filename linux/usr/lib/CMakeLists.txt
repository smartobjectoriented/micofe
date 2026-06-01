
add_library(zip STATIC zip.c)

set_target_properties(zip PROPERTIES COMPILE_FLAGS  "-std=gnu99")

# JSON Support
add_subdirectory(frozen)
