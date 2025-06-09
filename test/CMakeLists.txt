project(vkml_rt_test)

# Enable testing
enable_testing()

# List of test sources
set(VKML_TEST_SOURCES
    test_device.cpp
    test_logging.cpp
    test_program.cpp
    test_runtime.cpp
    test_shader.cpp
    test_storage.cpp
    test_utils.cpp
)

# Create test_utils_lib library
add_library(test_utils_lib test_utils.cpp)

# Include directories for test_utils_lib
target_include_directories(test_utils_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
)

# Create an executable and test for each source file
foreach(SOURCE ${VKML_TEST_SOURCES})
    # Get the name of the test (without extension)
    string(REPLACE ".cpp" "" TEST_NAME ${SOURCE})

    # Create an executable for the test
    add_executable(${TEST_NAME} ${SOURCE} init.cpp)

    # Link the executable to the vkml-rt library and test_utils_lib
    target_link_libraries(${TEST_NAME} PRIVATE vkml-rt test_utils_lib)

    # Include directories
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
    )

set_target_properties(${TEST_NAME} PROPERTIES
        INTDIR "${CMAKE_CURRENT_BINARY_DIR}/intermediate/${TEST_NAME}")
    # Add the test
    add_test(NAME vkml_runtime_test_${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Set test properties (optional, for organization in IDEs)
