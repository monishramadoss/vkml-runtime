#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>
#include "error_handling.h"

namespace runtime {

class MemoryManager {
public:
    std::shared_ptr<MemoryManager> create(VmaAllocator, VkDevice device);
   
    // Prevent copying
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    VkResult createBuffer(VkBufferCreateInfo& requirements,
                           VmaAllocationCreateInfo& createInfo,
                           VkBuffer* buffer,
                           VmaAllocation* allocation,
                           VmaAllocationInfo* allocationInfo);
    void destroyBuffer(VkBuffer* buffer, VmaAllocation* allocation);
    void freeMemory(VmaAllocation allocation);

    void mapMemory(VmaAllocation allocation, void** data);
    void unmapMemory(VmaAllocation allocation);
    
    VkResult copyToMemory(VmaAllocation dst, const void* src, 
                         size_t size, size_t dstOffset = 0);
    VkResult copyFromMemory(VmaAllocation src, void* dst, 
                           size_t size, size_t srcOffset = 0);

    VkResult flushCacheToMemory(VmaAllocation allocation, size_t offset, size_t size);
    VkResult invalidateCache(VmaAllocation allocation, size_t offset, size_t size);

    VkMemoryPropertyFlags getMemoryProperties(VmaAllocation allocation) const;

    VkDevice getDevice() const { return m_device; }
    VmaAllocator getAllocator() const { return m_allocator; }
    
private:

    explicit MemoryManager(VmaAllocator allocator, VkDevice device);
    ~MemoryManager();

    VmaAllocator m_allocator;
    VkDevice m_device;
};

} // namespace runtime

#endif // MEMORY_MANAGER_H
