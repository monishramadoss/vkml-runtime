#include "buffer_allocator.h"
#include "memory_manager.h"
#include <stdexcept>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace runtime {

static std::shared_ptr<BufferAllocator> create(VkDevice device, const std::shared_ptr<MemoryManager>& allocator) {
    return std::shared_ptr<BufferAllocator>(new BufferAllocator(device, allocator));
}

VkResult BufferAllocator::createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* buffer) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }
    else if(properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }
    else if(properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }
    
    VmaAllocation allocation;
    VkResult result = m_memory_manager->createBuffer(bufferInfo, allocInfo, buffer, &allocation, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }
    m_allocations.emplace(*buffer, allocation);
    return result;
}

void BufferAllocator::destroyBuffer(VkBuffer* buffer) {
    VmaAllocation allocation = m_allocations.at(*buffer);
    
    m_memory_manager->destroyBuffer(buffer, &allocation);
}

void* BufferAllocator::mapMemory(VkBuffer* buffer) {
    VmaAllocation allocation = m_allocations.at(*buffer);
    void* data;
    m_memory_manager->mapMemory(allocation, &data);
    return data;
}

void BufferAllocator::unmapMemory(VkBuffer* buffer) {
    VmaAllocation allocation = m_allocations.at(*buffer);
    m_memory_manager->unmapMemory(allocation);

}

void BufferAllocator::flushCacheToMemory(VkBuffer *buffer, size_t offset, size_t size) {
    VmaAllocation allocation = m_allocations.at(*buffer);
    m_memory_manager->flushCacheToMemory( allocation, offset, size);
}

void BufferAllocator::invalidateCache(VkBuffer *buffer, size_t offset, size_t size) {
    VmaAllocation allocation = m_allocations.at(*buffer);
    m_memory_manager->invalidateCache( allocation, offset, size);

}

VkDevice BufferAllocator::getDevice() const {
    return m_device;
}

BufferAllocator::BufferAllocator(VkDevice device, const std::shared_ptr<MemoryManager>& memory_manager)
    : m_device(device), m_memory_manager(memory_manager)
{
    m_initialized = true;   
}


void BufferAllocator::cleanup() {
    if (!m_initialized) {
        return;
    }
    
    for (auto& alloc : m_allocations) {
        VkBuffer buffer = alloc.first;
        VmaAllocation allocation = alloc.second;
        m_memory_manager->destroyBuffer(&buffer, &allocation);
    }
    m_allocations.clear();
    m_initialized = false;
}

BufferAllocator::~BufferAllocator() {
    cleanup();
}



} // namespace runtime


#