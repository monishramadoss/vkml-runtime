cmake_minimum_required(VERSION 3.16)
project(vkrt, VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VKRT_USE_VULKAN_SDK "Use Vulkan SDK" ON)
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)

add_subdirectory(ext/SPIRV-Reflect)
add_subdirectory(ext/volk)
add_subdirectory(ext/VulkanMemoryAllocator)
add_subdirectory(lib)


enable_testing()
add_subdirectory(test)