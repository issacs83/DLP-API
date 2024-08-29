# --------------------------------------
# Global configuration
# --------------------------------------
cmake_minimum_required (VERSION 3.13.1)
project(dlpc_api_lib)
find_library(CYUSBSERIAL cyusbserial third_party/cyusbserial)

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
target_link_libraries(dlpc347x ${CYUSBSERIAL})

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
target_link_libraries(dlpc654x ${CYUSBSERIAL})

