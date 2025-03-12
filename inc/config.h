#pragma once

/**
 * @file config.h
 * @brief Global configuration settings for the VKML Runtime
 * 
 * This file contains build-time configuration options for the ML runtime,
 * including memory allocator settings, validation options, and feature flags.
 */

//-----------------------------------------------------------------------------
// Vulkan Memory Allocator Configuration
//-----------------------------------------------------------------------------
// Use the statically linked Vulkan functions
#define VMA_STATIC_VULKAN_FUNCTIONS 0

// Don't use dynamically loaded Vulkan functions
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

// Configuration for memory allocation patterns
#define VMA_USE_STL_CONTAINERS 0
#define VMA_USE_STL_VECTOR 0
#define VMA_USE_STL_UNORDERED_MAP 0
#define VMA_USE_STL_LIST 0

// Include the VMA implementation in this compilation unit
#define VMA_IMPLEMENTATION

//-----------------------------------------------------------------------------
// Vulkan Configuration
//-----------------------------------------------------------------------------
// Use volk for Vulkan function loading
#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION

//-----------------------------------------------------------------------------
// Debug Configuration
//-----------------------------------------------------------------------------
#ifdef DEBUG
// Add padding around allocations to detect overflows
#define VMA_DEBUG_MARGIN 16

// Enable memory corruption detection
#define VMA_DEBUG_DETECT_CORRUPTION 1

// Enable verbose validation
#define VKML_VERBOSE_VALIDATION 1

// Track memory allocations
#define VKML_TRACK_MEMORY_USAGE 1
#endif

//-----------------------------------------------------------------------------
// Performance Configuration
//-----------------------------------------------------------------------------

// Threading model (0 = single-threaded, 1 = multi-threaded)
#ifndef VKML_THREADING_MODEL
#define VKML_THREADING_MODEL 1
#endif

// Maximum number of command buffers to pool per thread
#ifndef VKML_CMD_BUFFER_POOL_SIZE
#define VKML_CMD_BUFFER_POOL_SIZE 8
#endif

// Enable automatic memory defragmentation
#ifndef VKML_AUTO_DEFRAG
#define VKML_AUTO_DEFRAG 1
#endif
