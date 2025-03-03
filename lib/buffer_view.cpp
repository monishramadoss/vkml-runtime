#include "buffer_view.h"
#include <stdexcept>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>


namespace runtime {
    
    BufferView::BufferView(Buffer& buffer, size_t offset, size_t size)
        : m_buffer(buffer.getBuffer()), m_offset(offset), m_size(size)
    {
        if (m_offset + m_size > buffer.getSize()) {
            throw std::runtime_error("Buffer view size exceeds buffer size");
        }
    }

} // namespace runtime
