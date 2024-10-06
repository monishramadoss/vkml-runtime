
project(rt_test)

add_executable(rt_test
	rt.cpp
)

target_link_libraries(rt_test PRIVATE rt)
add_test(NAME rt_test COMMAND rt_test)
set_property(TARGET rt_test PROPERTY FOLDER test)