#include "memory_manager.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace runtime {

MemoryManager::MemoryManager(VmaAllocator allocator)
    : m_allocator(allocator)
{
}

VkResult MemoryManager::allocateMemory(const VkMemoryRequirements& requirements,
                                       const VmaAllocationCreateInfo& createInfo,
                                       VmaAllocation* allocation,
                                       VmaAllocationInfo* allocationInfo)
{
    return vmaAllocateMemory(m_allocator, &requirements, &createInfo, allocation, allocationInfo);
}

void MemoryManager::freeMemory(VmaAllocation allocation)
{
    vmaFreeMemory(m_allocator, allocation);
}

void MemoryManager::mapMemory(VmaAllocation allocation, void** data)
{
    vmaMapMemory(m_allocator, allocation, data);
}

void MemoryManager::unmapMemory(VmaAllocation allocation)
{
    vmaUnmapMemory(m_allocator, allocation);
}

VkResult MemoryManager::copyToMemory(VmaAllocation dst, const void* src, 
                                     size_t size, size_t dstOffset)
{
    return vmaMapMemory(m_allocator, dst, nullptr);
}

VkResult MemoryManager::copyFromMemory(VmaAllocation src, void* dst, 
                                       size_t size, size_t srcOffset)
{
    return vmaMapMemory(m_allocator, src, nullptr);
}

VkMemoryPropertyFlags MemoryManager::getMemoryProperties(VmaAllocation allocation) const
{
    VmaAllocationInfo info;
    vmaGetAllocationInfo(m_allocator, allocation, &info);
    return info.memoryType->flags;
}

} // namespace runtime
