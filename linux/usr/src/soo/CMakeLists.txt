

add_executable(injector injector.c)

add_executable(saveme saveme.c)
add_executable(restoreme restoreme.c)
add_executable(shutdownme shutdownme.c)

add_executable(melist melist.c)
add_executable(fb_mapper fb_mapper.c)
add_executable(input_forwarding input_forwarding.c)

add_subdirectory(emiso_engine)

target_link_libraries(saveme zip)
target_link_libraries(restoreme zip)
