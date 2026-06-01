
# CMake for building all agency usr applications

cmake_minimum_required(VERSION 3.20)

project(linux-usr LANGUAGES C CXX)
 
set(CXX_STANDARD 17)

include_directories(
	include
	lib
)

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

    set(CMAKE_C_FLAGS_DEBUG "-O0 -g -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer")

	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O0")

	include_directories(
		../rootfs/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/drm
		../rootfs/host/aarch64-buildroot-linux-gnu/sysroot/usr/include
	)

	link_directories(../../rootfs/host/aarch64-buildroot-linux-gnueabihf/sysroot/usr/lib/)
	 
else()

	set(CMAKE_C_FLAGS_DEBUG "-O0 -g -march=armv7-a -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -march=armv7-a -mfloat-abi=hard -mfpu=neon -Wno-psabi")

	set(CMAKE_CXX_FLAGS_DEBUG "-O0  -g -march=armv7-a -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3  -march=armv7-a -mfloat-abi=hard -mfpu=neon -Wno-psabi")

	link_directories(../../rootfs/host/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/)
endif()

if  (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

	# The following directories are considered during the build process.

	add_subdirectory(lib)
	add_subdirectory(src)

endif()
