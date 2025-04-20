
project(vkml_rt_test)

add_executable(init_test
	init.cpp
)

add_executable(shader_test 
	square.cpp
)

target_link_libraries(init_test PRIVATE vkml-rt)
target_link_libraries(shader_test PRIVATE vkml-rt)

add_test(NAME init_test COMMAND init_test)
add_test(NAME shader_test COMMAND shader_test)

set_property(TARGET init_test PROPERTY FOLDER test)
set_property(TARGET shader_test PROPERTY FOLDER test)