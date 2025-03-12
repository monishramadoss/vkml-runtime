#include "buffer_view.h"
#include "buffer.h"

#include <stdexcept>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace runtime {
    
    BufferView::BufferView(const std::shared_ptr<Buffer>& buffer, size_t offset, size_t size)
        : m_buffer(buffer), m_offset(offset), m_size(size == VK_WHOLE_SIZE ? buffer->getSize() - offset : size)
    {
        if (offset + m_size > buffer->getSize()) {   
            throw std::runtime_error("Buffer view size exceeds buffer size");
        }
        
        // Note: m_view is not initialized here since BufferView doesn't create a VkBufferView
        // This would require extending the implementation to create a VkBufferView if needed
        m_view = VK_NULL_HANDLE;
    }

} // namespace runtime
