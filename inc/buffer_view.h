#ifndef BUFFER_VIEW_H
#define BUFFER_VIEW_H
#include <vulkan/vulkan.h>
#include <cstddef>
#include <memory>

namespace runtime {

// Forward declarations
class Buffer;

class BufferView {
public:
    BufferView(const std::shared_ptr<Buffer>& buffer, size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    
    std::shared_ptr<Buffer> getBuffer() const { return m_buffer; }
    size_t getOffset() const { return m_offset; }
    size_t getSize() const { return m_size; }
    
private:
    std::shared_ptr<Buffer> m_buffer;
    VkBufferView m_view;
    size_t m_offset;
    size_t m_size;
};

} // namespace runtime

#endif // RUNTIME_BUFFER_VIEW_H