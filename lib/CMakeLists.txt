
if(VKRT_USE_VULKAN_SDK)
    find_package(Vulkan REQUIRED)

    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan SDK not found")
    endif()

endif()

if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

add_library( rt STATIC vkrt.cpp inc/vk_instance.h inc/vk_device.h inc/vk_buffer.h inc/vk_pgrm.h)
target_include_directories(rt PUBLIC inc)
add_library(rt::rt ALIAS rt)
target_include_directories( rt PRIVATE ext/volk)
target_include_directories( rt PRIVATE ext/VulkanMemoryAllocator/include)
target_include_directories( rt PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries( rt "${Vulkan_LIBRARIES}")
target_link_libraries( rt volk)
target_link_libraries( rt VulkanMemoryAllocator)
