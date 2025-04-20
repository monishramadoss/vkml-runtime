#ifndef CONFIG_H
#define CONFIG_H

#define VK_NO_PROTOTYPES
#define VOLK_VULKAN_VERSION VK_API_VERSION_1_4

#ifdef _DEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

// Disable performance counters to avoid hanging or crashes
#define DISABLE_PERFORMANCE_COUNTERS

//
//#ifdef WIN32
//#define VK_USE_PLATFORM_WIN32_KHR
//#elif __UNIX__
//#define VK_USE_PLATFORM_XLIB_KHR
//#endif 


#endif