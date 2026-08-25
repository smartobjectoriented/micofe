

add_executable(s3c-inject s3c-inject.c)

add_executable(s3c-save s3c-save.c)
add_executable(s3c-restore s3c-restore.c)
add_executable(s3c-shutdown s3c-shutdown.c)

add_executable(s3c-list s3c-list.c)

target_link_libraries(s3c-save zip)
target_link_libraries(s3c-restore zip)

# MICOFE additions: the framebuffer/input forwarders and the EMISO engine
add_executable(fb_mapper fb_mapper.c)
add_executable(input_forwarding input_forwarding.c)

add_subdirectory(emiso_engine)
