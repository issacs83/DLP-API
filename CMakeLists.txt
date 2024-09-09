# --------------------------------------
# Global configuration
# --------------------------------------
cmake_minimum_required (VERSION 3.13.1)
project(dlpc_api_lib)

# Find and link libusb-1.0 package
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Link the libusb library if found
if (LIBUSB_FOUND)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    link_directories(${LIBUSB_LIBRARY_DIRS})
    set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARIES})
else()
    message(FATAL_ERROR "libusb-1.0 library not found")
endif()

# Define the path to the third_party/cyusbserial directory
set(CYUSBSERIAL_DIR "${CMAKE_SOURCE_DIR}/third_party/cyusbserial")

# Include third_party/cyusbserial subdirectory to build cyusbserial
add_subdirectory(${CYUSBSERIAL_DIR})

include_directories(${CYUSBSERIAL_DIR})

# Set the output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

# Create the bin directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

set(common_files
    api/dlpc_common.h
    api/dlpc_common_private.h
    api/dlpc_common.c
    samples/cypress_i2c.h
    samples/cypress_i2c.c)

# --------------------------------------
# DLPC347x Library Configuration
# --------------------------------------
set(DLPC347x_files 
    api/dlpc34xx.h
    api/dlpc34xx_dual.h
    api/dlpc347x_internal_patterns.h
    api/dlpc34xx.c
    api/dlpc34xx_dual.c
    api/dlpc347x_internal_patterns.c
    samples/dlpc347x_samples.c
    )

add_library(dlpc347x 
    ${common_files}
    ${DLPC347x_files}
)

target_include_directories(dlpc347x PRIVATE api)
target_include_directories(dlpc347x PRIVATE samples)
target_include_directories(dlpc347x PRIVATE third_party)
target_link_libraries(dlpc347x ${LIBUSB_LIBRARIES} cyusbserial)

# --------------------------------------
# DLPC654x Library Configuration
# --------------------------------------
set(DLPC654x_files 
    api/dlpc654x.h
    api/dlpc654x.c
    )

add_library(dlpc654x 
    ${common_files}
    ${DLPC654x_files}
)

target_include_directories(dlpc654x PRIVATE api)
target_include_directories(dlpc654x PRIVATE samples)
target_include_directories(dlpc654x PRIVATE third_party)
target_link_libraries(dlpc654x ${LIBUSB_LIBRARIES})

# Add DLPC34XX and DLPC_COMMON source files
set(DLPC34XX_files
    api/dlpc34xx.c
    api/dlpc34xx_dual.c
    api/dlpc347x_internal_patterns.c)

set(DLPC_COMMON_files
    api/dlpc_common.c)

set(sample_files
    samples/cypress_i2c.c
    samples/dlpc347x_samples.c)

# Create executable for the sample files
add_executable(test_samples ${sample_files} ${DLPC34XX_files} ${DLPC_COMMON_files})

# Include necessary directories
target_include_directories(test_samples PRIVATE api samples)

# Link the shared version of libcyusbserial.so
target_link_libraries(test_samples /home/issacs/texasinstruments/DLP-API/third_party/cyusbserial/libcyusbserial.so ${LIBUSB_LIBRARIES} pthread m)
# target_link_libraries(test_samples cyusbserial ${LIBUSB_LIBRARIES} pthread m)
