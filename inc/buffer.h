#ifndef BUFFER_H
#define BUFFER_H

#include <memory>
#include <vulkan/vulkan.h>
#include "error_handling.h"

namespace runtime {

class BufferAllocator;

class Buffer {
public:
    friend static std::shared_ptr<Buffer> createStorageBuffer(uint32_t bufferInfo, bool isHostVisible);
    friend static std::shared_ptr<Buffer> createUniformBuffer(uint32_t bufferInfo, bool isHostVisible);

    const size_t getSize() const { return m_size; }
    const VkBuffer getBuffer() const { return m_buffer; }
    const VkBufferUsageFlags getUsage() const { return m_usage; }
    const bool isMemoryBound() const { return m_memoryBound; }

protected:
    Buffer(const std::shared_ptr<BufferAllocator> &allocator,
    VkDeviceSize size,
    VkMemoryPropertyFlags properties,
    VkBufferUsageFlags usage);

    virtual ~Buffer();
    
    std::shared_ptr<BufferAllocator> m_allocator;
    VkBuffer m_buffer {nullptr};
    VkDeviceSize m_size{0};
    VkBufferUsageFlags m_usage{0};
    bool m_memoryBound{false};
};

} // namespace runtime

#endif // BUFFER_H