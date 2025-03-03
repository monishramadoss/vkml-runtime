#pragma once
#include "buffer.h"
#include <vector>

namespace runtime {

class SparseBuffer : public Buffer {
public:
    static std::shared_ptr<SparseBuffer> create(
        std::shared_ptr<BufferAllocator> allocator,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        const std::vector<uint32_t>& sparseQueueFamilies);

    ~SparseBuffer();

    struct MemoryBinding {
        VkDeviceMemory memory;
        VkDeviceSize memoryOffset;
        VkDeviceSize resourceOffset;
        VkDeviceSize size;
    };

    void bindMemory(const std::vector<MemoryBinding>& bindings);
    bool isSparseBinding() const { return m_sparseBinding; }

private:
    SparseBuffer(std::shared_ptr<BufferAllocator> allocator,
                 VkDeviceSize size,
                 VkBufferUsageFlags usage,
                 const std::vector<uint32_t>& sparseQueueFamilies);

    void initializeSparse();

    bool m_sparseBinding{false};
    std::vector<uint32_t> m_sparseQueueFamilies;
    std::vector<VkSparseMemoryBind> m_memoryBindings;
};

} // namespace runtime
