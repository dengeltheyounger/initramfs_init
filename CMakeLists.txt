cmake_minimum_required(VERSION 3.10)

# set the project name
project(init_project)

# We'll assume C99 standard

set(CMAKE_C_STANDARD 99)

# Make the executable from init.c and switch_root.c

add_executable(init init.c logger.c cleanup.c password.c utilities.c
	switch_root.c secure_getline.c)

add_compile_options(-ldl -lpthread)

# link the required libraries

target_link_libraries(init /usr/src/initramfs/lib64/liblvm2app.so.2.2
			/usr/src/initramfs/lib64/liblvm2cmd.so.2.02
			/usr/src/initramfs/lib64/libdevmapper.so.1.02
			/usr/src/initramfs/lib64/libdevmapper-event.so.1.02
			/usr/src/initramfs/lib64/libudev.so.1
			/usr/src/initramfs/lib64/libselinux.so.1
			/usr/src/initramfs/lib64/libcryptsetup.so.12
			/usr/src/initramfs/lib64/libjson-c.so.5
			/usr/src/initramfs/lib64/libargon2.so.1
			/usr/src/initramfs/lib64/libpthread.so.0
			/usr/src/initramfs/lib64/libdl.so.2
			/usr/lib64/libdaemonclient.a
			/usr/src/initramfs/lib64/libaio.so.1
			/usr/src/initramfs/lib64/libblkid.so.1
			/usr/src/initramfs/lib64/libpcre.so.1
			/usr/src/initramfs/lib64/libc.so.6
			/usr/src/initramfs/lib64/libm.so.6
			/usr/src/initramfs/lib64/ld-linux-x86-64.so.2
			/usr/src/initramfs/lib64/libuuid.so.1
			/usr/src/initramfs/lib64/libcrypto.so.1.1
			/usr/src/initramfs/lib64/libz.so.1)
