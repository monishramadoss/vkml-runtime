#include "buffer.h"
#include "buffer_allocator.h"
#include "buffer_view.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>

namespace runtime {

std::shared_ptr<Buffer> Buffer::createStorageBuffer(
    std::shared_ptr<BufferAllocator> allocator,
    VkDeviceSize size,
    VkBufferUsageFlags additionalUsage,
    bool hostVisible)
{
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | additionalUsage;
    return std::make_shared<Buffer>(allocator, size, usage, hostVisible);
}

std::shared_ptr<Buffer> Buffer::createUniformBuffer(
    std::shared_ptr<BufferAllocator> allocator,
    VkDeviceSize size,
    bool hostVisible)
{
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    return std::make_shared<Buffer>(allocator, size, usage, hostVisible);
}

Buffer::~Buffer()
{
    if (m_buffer != VK_NULL_HANDLE) {
        m_allocator->freeBuffer(m_buffer, m_allocation);
    }

    m_buffer = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
}

void* Buffer::map()
{
    if (!m_mapped) {
        m_allocator->getMemoryManager()->mapMemory(m_allocation, &m_mappedData);
        m_mapped = true;
    }

    return m_mappedData;
}

void Buffer::unmap()
{
    if (m_mapped) {
        m_allocator->getMemoryManager()->unmapMemory(m_allocation);
        m_mapped = false;
    }
}

void Buffer::flush(VkDeviceSize offset, VkDeviceSize size)
{
    if (m_mapped) {
        m_allocator->getMemoryManager()->flushAllocation(m_allocation, offset, size);
    }
}

void Buffer::invalidate(VkDeviceSize offset, VkDeviceSize size)
{
    if (m_mapped) {
        m_allocator->getMemoryManager()->invalidateAllocation(m_allocation, offset, size);
    }

    m_mappedData = nullptr;
}

void Buffer::copyFrom(const void* data, VkDeviceSize size, VkDeviceSize offset)

{
    if (m_mapped) {
        std::memcpy(static_cast<char*>(m_mappedData) + offset, data, size);
    } else {
        void* mappedData = map();
        std::memcpy(static_cast<char*>(mappedData) + offset, data, size);
    }

    flush(offset, size);
}

void Buffer::copyTo(void* data, VkDeviceSize size, VkDeviceSize offset)
{
    if (m_mapped) {
        std::memcpy(data, static_cast<char*>(m_mappedData) + offset, size);
    } else {
        void* mappedData = map();
        std::memcpy(data, static_cast<char*>(mappedData) + offset, size);
    }
}

BufferView Buffer::getView(VkDeviceSize offset, VkDeviceSize size)
{
    return BufferView(*this, offset, size);
}

bool Buffer::isHostVisible() const
{
    return m_allocationInfo.pUserData != nullptr;
}

Buffer::Buffer(std::shared_ptr<BufferAllocator> allocator,
                VkDeviceSize size,
                VkBufferUsageFlags usage,
                bool hostVisible)
    : m_allocator(allocator),
        m_size(size),
        m_usage(usage)
{
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (hostVisible) {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    m_allocator->allocateBuffer(bufferInfo, allocInfo, m_buffer, m_allocation, m_allocationInfo);
}

void Buffer::initialize()
{
    if (m_mapped) {
        m_mappedData = m_allocator->getMemoryManager()->mapMemory(m_allocation);
    }   
}

void Buffer::cleanup()
{
    if (m_mapped) {
        m_allocator->getMemoryManager()->unmapMemory(m_allocation);
    }
}

} // namespace runtime