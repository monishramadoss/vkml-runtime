
project(vkml_rt_test)

# Add unit tests
add_executable(program_test
    unit/program_test_simple.cpp
)

# Original tests
add_executable(init_test
    init.cpp
)

add_executable(shader_test 
    square.cpp
)

# Link libraries
target_link_libraries(program_test PRIVATE vkml-rt)
target_link_libraries(init_test PRIVATE vkml-rt)
target_link_libraries(shader_test PRIVATE vkml-rt)

# Include directories
target_include_directories(program_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/inc
)

# Register tests with CTest
add_test(NAME program_test COMMAND program_test)
add_test(NAME init_test COMMAND init_test)
add_test(NAME shader_test COMMAND shader_test)

# Group tests
set_property(TARGET program_test PROPERTY FOLDER test)
set_property(TARGET init_test PROPERTY FOLDER test)
set_property(TARGET shader_test PROPERTY FOLDER test)