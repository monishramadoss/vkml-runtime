cmake_minimum_required(VERSION 3.16)
project(vkrt, VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VKRT_USE_VULKAN_SDK "Use Vulkan SDK" ON)
option(ENABLE_TEST "Enable Tests" ON)

if(VKRT_USE_VULKAN_SDK)
    find_package(Vulkan REQUIRED)
    if(NOT Vulkan_FOUND)
        set(VKRT_USE_VULKAN_SDK OFF)
        message( "Vulkan SDK not found")
    else ()
        #set(ENV{VULKAN_SDK} ENV{VK_LAYER_PATH})
        message( "Vulkan SDK found: ${Vulkan_FOUND}")
        message( "Vulkan SDK path: ${Vulkan_INCLUDE_DIRS}")
    endif()
endif()

if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)

add_subdirectory(ext/SPIRV-Reflect)
add_subdirectory(ext/volk)
add_subdirectory(ext/VulkanMemoryAllocator)

set(        
    EXT_HEADERS 
    volk_headers
    ext/VulkanMemoryAllocator/include
    ext/SPIRV-Reflect
    ${Vulkan_INCLUDE_DIRS}
)

include_directories(inc)
file(GLOB_RECURSE VKRT_HEADERS inc/*.h)
file(GLOB_RECURSE VKRT_SOURCES lib/*.cpp)

add_library(vkml-rt STATIC ${VKRT_HEADERS} ${VKRT_SOURCES})
target_include_directories(vkml-rt PUBLIC ${EXT_HEADERS})

target_link_libraries(vkml-rt PUBLIC volk)
target_link_libraries(vkml-rt PUBLIC VulkanMemoryAllocator)
target_link_libraries(vkml-rt PUBLIC spirv-reflect-static)


set_property(GLOBAL PROPERTY VKRT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/inc ${EXT_HEADERS})

# if(ENABLE_TEST)
    add_subdirectory(test)
# endif()