#ifndef SPARSE_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <stdexcept>
#include "buffer.h"
#include "logging.h"
#include "error_handling.h"

namespace runtime {

/**
 * @brief Implements a sparse buffer that allows non-contiguous memory bindings
 * 
 * Sparse buffers allow for more flexible memory management, including:
 * - Partial resource binding
 * - Memory aliasing
 * - On-demand memory allocation
 * - Optimized memory usage for large, partially populated resources
 */
class SparseBuffer : public Buffer {
public:
    
    struct MemoryBinding {
        VkDeviceMemory memory;       ///< Device memory handle
        VkDeviceSize memoryOffset;   ///< Offset within the memory allocation
        VkDeviceSize resourceOffset; ///< Offset within the buffer resource
        VkDeviceSize size;           ///< Size of the memory binding
        VkSparseMemoryBindFlags flags; ///< Flags for the memory binding
    };

  
    static std::shared_ptr<SparseBuffer> create(
        std::shared_ptr<BufferAllocator> allocator,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        const std::vector<uint32_t>& sparseQueueFamilies);

    ~SparseBuffer();

    
    void bindMemory(const std::vector<MemoryBinding>& bindings, VkQueue sparseQueue, VkFence fence);

    VkDeviceSize getPageSize() const { return m_pageSize; }

    bool isSparseBinding() const noexcept { return m_sparseBinding; }
    std::vector<VkSparseMemoryBind> getMemoryBindings() const noexcept { 
        return m_memoryBindings; 
    }

    bool isRangeBound(VkDeviceSize offset, VkDeviceSize size) const;

protected:
    SparseBuffer(std::shared_ptr<BufferAllocator> allocator,
                 VkDeviceSize size,
                 VkBufferUsageFlags usage,
                 const std::vector<uint32_t>& sparseQueueFamilies);

private:

    void validateFeatureSupport();

    bool m_sparseBinding{false};
    std::vector<uint32_t> m_sparseQueueFamilies;
    std::vector<VkSparseMemoryBind> m_memoryBindings;
    VkPhysicalDeviceProperties m_deviceProperties{};
    VkDeviceSize m_pageSize{0};
};

} // namespace runtime

#endif // SPARSE_BUFFER_H