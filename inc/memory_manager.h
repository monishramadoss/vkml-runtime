#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>
#include "../error_handling.h"

namespace runtime {

class MemoryManager {
public:
    explicit MemoryManager(VmaAllocator allocator, VkDevice device)
        : m_allocator(allocator), m_device(device) {}
    
    ~MemoryManager() = default;

    // Prevent copying
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    VkResult allocateMemory(const VkMemoryRequirements& requirements,
                           const VmaAllocationCreateInfo& createInfo,
                           VmaAllocation* allocation,
                           VmaAllocationInfo* allocationInfo);

    void freeMemory(VmaAllocation allocation);

    void mapMemory(VmaAllocation allocation, void** data);
    void unmapMemory(VmaAllocation allocation);

    VkResult copyToMemory(VmaAllocation dst, const void* src, 
                         size_t size, size_t dstOffset = 0);
    VkResult copyFromMemory(VmaAllocation src, void* dst, 
                           size_t size, size_t srcOffset = 0);

    VkMemoryPropertyFlags getMemoryProperties(VmaAllocation allocation) const;

    VkDevice getDevice() const { return m_device; }
    VmaAllocator getAllocator() const { return m_allocator; }
    
private:
    VmaAllocator m_allocator;
    VkDevice m_device;
};

} // namespace runtime
