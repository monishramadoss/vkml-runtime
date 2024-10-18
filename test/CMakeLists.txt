
project(rt_test)

add_executable(rt_test
	rt.cpp
)
add_executable(shader_test 
	ssprgm.cpp
)

target_link_libraries(rt_test PRIVATE rt)
target_link_libraries(shader_test PRIVATE rt)
add_test(NAME rt_test COMMAND rt_test)
add_test(NAME shader_test COMMAND shader_test)

set_property(TARGET rt_test PROPERTY FOLDER test)
set_property(TARGET shader_test PROPERTY FOLDER test)