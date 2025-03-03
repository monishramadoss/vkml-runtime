#pragma once

// VulkanMemoryAllocator configuration
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_USE_STL_CONTAINERS 0
#define VMA_USE_STL_VECTOR 0
#define VMA_USE_STL_UNORDERED_MAP 0
#define VMA_USE_STL_LIST 0
#define VMA_IMPLEMENTATION

// Vulkan configuration
#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION

#ifdef DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif
