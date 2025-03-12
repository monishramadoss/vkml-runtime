#include "buffer.h"
#include "buffer_allocator.h"
#include "buffer_view.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <memory>
#include <stdexcept>

namespace runtime {
    Buffer::Buffer(const std::shared_ptr<BufferAllocator>& allocator,
                   VkDeviceSize size,
                   VkMemoryPropertyFlags properties,
                   VkBufferUsageFlags usage)
        : m_allocator(allocator)
        , m_size(size)
        , m_usage(usage) {
        m_allocator->createBuffer(size, usage, properties, &m_buffer);        
    }

} // namespace runtime
