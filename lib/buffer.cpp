#include "buffer.h"
#include "buffer_allocator.h"
#include "buffer_view.h"
#include "vulkan_utils.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <stdexcept>

namespace runtime {

Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice)
    : m_device(device), m_physicalDevice(physicalDevice), 
      m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), 
      m_mappedData(nullptr), m_size(0) {
}

Buffer Buffer::createStorageBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                  VkDeviceSize size, const void* data) {
    Buffer buffer(device, physicalDevice);
    // Implementation details
    return buffer;
}

Buffer Buffer::createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                  VkDeviceSize size, const void* data) {
    Buffer buffer(device, physicalDevice);
    // Implementation details
    return buffer;
}

Buffer::~Buffer() {
    if (m_buffer != VK_NULL_HANDLE) {
        // Cleanup logic
    }
}

void* Buffer::map() {
    if (m_mappedData == nullptr) {
        // Mapping logic
    }
    return m_mappedData;
}

void Buffer::unmap() {
    if (m_mappedData != nullptr) {
        // Unmapping logic
        m_mappedData = nullptr;
    }
}

void Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
    // Implementation for flushing memory
}

std::shared_ptr<Buffer> Buffer::create(
    std::shared_ptr<BufferAllocator> allocator,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties) {
    
    // Create the buffer
    auto buffer = std::shared_ptr<Buffer>(new Buffer(allocator, size, usage, properties));
    
    // Initialize the buffer
    buffer->initialize(properties);
    
    // Don't access m_mapped directly in a static method
    // Instead of: if (m_mapped) { ... }
    // Use: if (buffer->m_mapped) { ... }
    
    return buffer;    
}

Buffer::Buffer(std::shared_ptr<BufferAllocator> allocator,
                VkDeviceSize size, 
                VkBufferUsageFlags usage,
                bool hostVisible)
    : m_allocator(allocator), 
      m_size(size),
      m_usage(usage) {
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

void Buffer::initialize(VkMemoryPropertyFlags properties) {
    if (this->m_mapped) {
        m_allocator->getMemoryManager()->unmapMemory(m_allocation);
    }
}

void Buffer::cleanup() {
    if (this->m_mapped) {
        m_allocator->getMemoryManager()->unmapMemory(m_allocation);
    }
}

void Buffer::unmap() {
    if (m_mapped) {
        m_allocator->unmap(m_allocation);
        m_mapped = false;
        m_offset = 0;
    }
}

void* Buffer::map() {
    if (!m_mapped) {
        m_mappedData = m_allocator->mapMemory(m_allocation);
        m_mapped = true;
    }

    return m_mappedData;
}

void Buffer::invalidate(VkDeviceSize offset, VkDeviceSize size) {
    if (m_mapped) {
        m_allocator->invalidateCache(m_allocation, offset, size);
    }
}

void Buffer::flush(VkDeviceSize offset, VkDeviceSize size) {
    if (m_mapped) {
        m_allocator->flushCacheToMemory(m_allocation, offset, size);
    }
}

void Buffer::copyFrom(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (m_mapped) {
        std::memcpy(static_cast<char*>(m_mappedData) + offset, data, size);
    } else {
        void* mappedData = map();
        std::memcpy(static_cast<char*>(mappedData) + offset, data, size);
    }

    flush(offset, size);
}

void Buffer::copyTo(void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (m_mapped) {
        std::memcpy(data, static_cast<char*>(m_mappedData) + offset, size);
    } else {
        void* mappedData = map();
        std::memcpy(data, static_cast<char*>(mappedData) + offset, size);
    }
}

BufferView Buffer::getView(VkDeviceSize offset, VkDeviceSize size) {
    return BufferView(*this, offset, size);
}

bool Buffer::isHostVisible() const {
    return m_allocationInfo.pUserData != nullptr;
}

} // namespace runtime