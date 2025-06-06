cmake_minimum_required(VERSION 3.15)

# This is a header-only library, so we'll use an interface target
add_library(test_common INTERFACE)

# Include directories for this target
target_include_directories(test_common INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/inc
)

# Link against the main library
target_link_libraries(test_common INTERFACE vkml-rt)
