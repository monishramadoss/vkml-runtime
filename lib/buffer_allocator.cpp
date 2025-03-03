#include "buffer_allocator.h"
#include <stdexcept>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace runtime {

BufferAllocator::BufferAllocator(std::shared_ptr<MemoryManager> memoryManager)
    : m_memoryManager(memoryManager)
{
    m_device = m_memoryManager->getDevice();
}

BufferAllocator::AllocationResult BufferAllocator::allocateBuffer(const VkBufferCreateInfo& bufferInfo,
                                                                  const VmaAllocationCreateInfo& allocInfo)
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    VkResult result = vmaCreateBuffer(m_memoryManager->getAllocator(), &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer");
    }

    return {buffer, allocation, allocationInfo};
}

void BufferAllocator::freeBuffer(VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(m_memoryManager->getAllocator(), buffer, allocation);
}

} // namespace runtime


