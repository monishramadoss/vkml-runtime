
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()


add_library( vkml-rt STATIC vkrt.cpp  inc/vkrt.h  inc/vka.h inc/vkb.h inc/vkc.h inc/vkd.h inc/vkt.h)
target_include_directories(vkml-rt PUBLIC ${EXT_HEADERS})
