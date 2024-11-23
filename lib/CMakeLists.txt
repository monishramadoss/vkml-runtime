
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()


add_library( vkml-rt STATIC vkrt.cpp  inc/vkrt.h  inc/vka.h inc/vkb.h inc/vkc.h inc/vkd.h inc/vkt.h)
target_include_directories( vkml-rt PUBLIC inc)
target_include_directories( vkml-rt PRIVATE ext/volk)
target_include_directories( vkml-rt PRIVATE ext/VulkanMemoryAllocator/include)
target_include_directories( vkml-rt PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_link_libraries( vkml-rt "${Vulkan_LIBRARIES}")
target_link_libraries( vkml-rt volk)
target_link_libraries( vkml-rt VulkanMemoryAllocator)
target_link_libraries( vkml-rt spirv-reflect-static)