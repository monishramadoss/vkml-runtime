project(vkml_rt_test)

# Enable testing
enable_testing()

# List of test sources using the custom CTest-compatible framework
set(VKML_TEST_SOURCES
    init.cpp
    test_device.cpp
    test_logging.cpp
    test_program.cpp
    test_runtime.cpp
    test_shader.cpp
    test_storage.cpp
    test_utils.cpp
)

# Add one executable for all tests
add_executable(vkml_runtime_tests ${VKML_TEST_SOURCES})

# Properly link the parent vkml-rt library and include directories
# (Assumes vkml-rt is built as a library target in the parent CMake project)
target_link_libraries(vkml_runtime_tests PRIVATE vkml-rt)
target_include_directories(vkml_runtime_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

# Register each test source as a separate CTest (using the same executable, but with a test filter argument)
# This assumes your test framework supports filtering by test name (e.g., via argv[1])
add_test(NAME vkml_runtime_test_device   COMMAND $<TARGET_FILE:vkml_runtime_tests> Device)
add_test(NAME vkml_runtime_test_logging  COMMAND $<TARGET_FILE:vkml_runtime_tests> Logging)
add_test(NAME vkml_runtime_test_program  COMMAND $<TARGET_FILE:vkml_runtime_tests> Program)
add_test(NAME vkml_runtime_test_runtime  COMMAND $<TARGET_FILE:vkml_runtime_tests> Runtime)
add_test(NAME vkml_runtime_test_shader   COMMAND $<TARGET_FILE:vkml_runtime_tests> Shader)
add_test(NAME vkml_runtime_test_storage  COMMAND $<TARGET_FILE:vkml_runtime_tests> Storage)

# Optionally, keep a catch-all test for running all tests
add_test(NAME vkml_runtime_tests_all COMMAND $<TARGET_FILE:vkml_runtime_tests>)

# Set test properties (optional, for organization in IDEs)
set_property(TARGET vkml_runtime_tests PROPERTY FOLDER test)
