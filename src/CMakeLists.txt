

if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

include_directories(${EXT_HEADERS})
add_library( vkml-rt STATIC "runtime.cpp"  "inc/runtime.h"  inc/vka.h inc/vkb.h inc/vkc.h inc/vkd.h inc/vke.h inc/vkt.h)
target_include_directories(vkml-rt PUBLIC ${EXT_HEADERS})
# target_include_directories(vkml-rt PRIVATE ext/VulkanMemoryAllocator/include)
target_link_libraries(vkml-rt PUBLIC volk)
target_link_libraries(vkml-rt PUBLIC VulkanMemoryAllocator)
target_link_libraries(vkml-rt PUBLIC spirv-reflect-static)