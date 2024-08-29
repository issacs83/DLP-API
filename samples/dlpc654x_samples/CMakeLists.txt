# --------------------------------------
# Global configuration
# --------------------------------------
cmake_minimum_required (VERSION 3.11.1)
project(dlpc_654x_samples)

SET(PLATFORM "32" CACHE STRING "Build type")

# need to find comparable libraries for Linux
IF(WIN32)
	SET(LIBNVUSB_SOURCES usb/win32/libnvusb.c include/win32/libnvusb.h)
	INCLUDE_DIRECTORIES( include/win32/winusb )
	find_library(WINUSB_LIB winusb HINTS C:/WinDDK/7600.16385.1/lib/win7/amd64)
	find_library(SETUPAPI_LIB setupapi HINTS C:/WinDDK/7600.16385.1/lib/win7/amd64)
	SET(USBLIBS ${WINUSB_LIB} ${SETUPAPI_LIB})

	find_library(SHLWAPI Shlwapi.lib) 

	set(platform_source 
		win_io.c
		win_io.h)
ENDIF(WIN32)

IF(UNIX)
	#FIND_PACKAGE(LibUSB REQUIRED)

    find_path(LIBUSB_INCLUDE_DIR
        NAMES libusb.h
        PATH_SUFFIXES "include" "libusb" "libusb-1.0")

	set(platform_source 
		unix_io.c
		unix_io.h)

ENDIF(UNIX)

set(library_source
    ../../api/dlpc_common.h
    ../../api/dlpc_common.c
    ../../api/dlpc_common_private.h
    ../../api/dlpc654x.h
    ../../api/dlpc654x.c
	dlpc654x_sample.c)

set(exe_source
    ${library_source}
    main.c
)

set(bootloader_update_x54x_source
    ${library_source}
    bootloader_update_x54x.c
    x54x_bootloader.h
)

#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}")
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM}")

add_executable(dlpc654x_sample 
    ${exe_source}
    ${platform_source}
)

add_executable(bootloader_update_x54x 
    ${bootloader_update_x54x_source}
    ${platform_source}
)

add_library(libdlpc654x_sample SHARED
    ${library_source}
    ${platform_source}
)

add_library(libdlpc654x_sample_s STATIC
    ${library_source}
    ${platform_source}
)

IF(UNIX)

# On Ubuntu (and maybe others), libusb.h gets placed in /usr/include/libusb-1.0
target_include_directories(dlpc654x_sample PRIVATE "/usr/include/libusb-1.0")
target_include_directories(bootloader_update_x54x PRIVATE "/usr/include/libusb-1.0")
target_include_directories(libdlpc654x_sample PRIVATE "/usr/include/libusb-1.0")
target_include_directories(libdlpc654x_sample_s PRIVATE "/usr/include/libusb-1.0")

# On Unix/Linux, pass "-DPLATFORM=32" or "-DPLATFORM=64" to CMake invocation to change build type
set_target_properties(dlpc654x_sample PROPERTIES COMPILE_OPTIONS "-m${PLATFORM}" LINK_FLAGS "-m${PLATFORM}")
set_target_properties(bootloader_update_x54x PROPERTIES COMPILE_OPTIONS "-m${PLATFORM}" LINK_FLAGS "-m${PLATFORM}")
set_target_properties(libdlpc654x_sample PROPERTIES COMPILE_OPTIONS "-m${PLATFORM}" LINK_FLAGS "-m${PLATFORM}")
set_target_properties(libdlpc654x_sample_s PROPERTIES COMPILE_OPTIONS "-m${PLATFORM}" LINK_FLAGS "-m${PLATFORM}")


# Unix/Linux use https://github.com/libusb/libusb for USB communications
# libusb-1.0-0-dev must be installed
# (sudo apt install libusb-1.0-0-dev)
target_link_libraries(dlpc654x_sample usb-1.0 udev)
target_link_libraries(bootloader_update_x54x usb-1.0 udev)
target_link_libraries(libdlpc654x_sample usb-1.0 udev)
target_link_libraries(libdlpc654x_sample_s usb-1.0 udev)
ENDIF(UNIX)

target_link_libraries(dlpc654x_sample ${USBLIBS})
target_link_libraries(dlpc654x_sample ${SHLWAPI})

target_link_libraries(bootloader_update_x54x ${USBLIBS})
target_link_libraries(bootloader_update_x54x ${SHLWAPI})

target_link_libraries(libdlpc654x_sample ${USBLIBS})
target_link_libraries(libdlpc654x_sample ${SHLWAPI})

target_link_libraries(libdlpc654x_sample_s ${USBLIBS})
target_link_libraries(libdlpc654x_sample_s ${SHLWAPI})
