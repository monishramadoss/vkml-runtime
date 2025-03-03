#pragma once
#include <vulkan/vulkan.h>
#include <cstddef>

namespace runtime {

// Forward declarations
class Buffer;

class BufferView {
public:
    BufferView(Buffer& buffer, size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    
    VkBuffer getBuffer() const { return m_buffer; }
    size_t getOffset() const { return m_offset; }
    size_t getSize() const { return m_size; }
    
private:
    VkBufferView m_view;
    size_t m_offset;
    size_t m_size;
};

} // namespace runtime
