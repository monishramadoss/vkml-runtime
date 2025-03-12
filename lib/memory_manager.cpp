#include "memory_manager.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstring>
#include <stdexcept>

namespace runtime {

    std::shared_ptr<MemoryManager> MemoryManager::create(VmaAllocator allocator, VkDevice device)
    {
        return std::shared_ptr<MemoryManager>(new MemoryManager(allocator, device));
    }
    
VkResult MemoryManager::createBuffer(VkBufferCreateInfo& bufferInfo,
                                       VmaAllocationCreateInfo& createInfo,
                                       VkBuffer* buffer,
                                       VmaAllocation* allocation,
                                       VmaAllocationInfo* allocationInfo)
{
    return vmaCreateBuffer(this->m_allocator, bufferInfo, createInfo, buffer, allocation, allocationInfo);
}

void MemoryManager::destroyBuffer(VkBuffer* buffer, VmaAllocation* allocation)
{
    if (buffer != nullptr && allocation != nullptr) {
        vmaDestroyBuffer(m_allocator, *buffer, *allocation);
        *buffer = VK_NULL_HANDLE;
        *allocation = VK_NULL_HANDLE;
    }
}

void MemoryManager::freeMemory(VmaAllocation allocation)
{
    if (allocation != VK_NULL_HANDLE) {
        vmaFreeMemory(m_allocator, allocation);
    }
}

void MemoryManager::mapMemory(VmaAllocation allocation, void** data)
{
    if (allocation == VK_NULL_HANDLE) {
        throw std::runtime_error("Attempted to map null allocation");
    }
    
    VkResult result = vmaMapMemory(m_allocator, allocation, data);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to map memory");
    }
}

void MemoryManager::unmapMemory(VmaAllocation allocation)
{
    if (allocation != VK_NULL_HANDLE) {
        vmaUnmapMemory(m_allocator, allocation);
    }
}

VkResult MemoryManager::copyToMemory(VmaAllocation dst, const void* src, 
                                     size_t size, size_t dstOffset)
{
    if (dst == VK_NULL_HANDLE || src == nullptr || size == 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    void* mappedData = nullptr;
    VkResult result = vmaMapMemory(m_allocator, dst, &mappedData);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    memcpy(static_cast<char*>(mappedData) + dstOffset, src, size);
    vmaUnmapMemory(m_allocator, dst);
    
    return VK_SUCCESS;
}

VkResult MemoryManager::copyFromMemory(VmaAllocation src, void* dst, 
                                       size_t size, size_t srcOffset)
{
    if (src == VK_NULL_HANDLE || dst == nullptr || size == 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    void* mappedData = nullptr;
    VkResult result = vmaMapMemory(m_allocator, src, &mappedData);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    memcpy(dst, static_cast<char*>(mappedData) + srcOffset, size);
    vmaUnmapMemory(m_allocator, src);
    
    return VK_SUCCESS;
}

VkResult MemoryManager::flushCacheToMemory(VmaAllocation allocation, size_t offset, size_t size)
{
    return vmaFlushAllocation(m_allocator, allocation, offset, size);
}

VkResult MemoryManager::invalidateCache(VmaAllocation allocation, size_t offset, size_t size)
{
    return vmaInvalidateAllocation(m_allocator, allocation, offset, size);
}

VkMemoryPropertyFlags MemoryManager::getMemoryProperties(VmaAllocation allocation) const
{
    if (allocation == VK_NULL_HANDLE) {
        return 0;
    }
    
    VmaAllocationInfo info;
    vmaGetAllocationInfo(m_allocator, allocation, &info);
    
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(m_allocator, info.memoryType, &flags);
    
    return flags;
}


MemoryManager::MemoryManager(VmaAllocator allocator, VkDevice device)
    : m_allocator(allocator)
    , m_device(device)
{
}

MemoryManager::~MemoryManager() {
    // Clean up any remaining resources
    
}

