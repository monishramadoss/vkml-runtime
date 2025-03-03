#pragma once
#include "memory_manager.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vk_mem_alloc.h>
namespace runtime {
    class BufferAllocator {
    public:
        explicit BufferAllocator(std::shared_ptr<MemoryManager> memoryManager);
        ~BufferAllocator() = default;

        // Prevent copying
        BufferAllocator(const BufferAllocator&) = delete;
        BufferAllocator& operator=(const BufferAllocator&) = delete;

        struct AllocationResult {
            VkBuffer buffer;
            VmaAllocation allocation;
            VmaAllocationInfo allocationInfo;
        };

        AllocationResult allocateBuffer(const VkBufferCreateInfo& bufferInfo,
                                    const VmaAllocationCreateInfo& allocInfo);

        void freeBuffer(VkBuffer buffer, VmaAllocation allocation);

        std::shared_ptr<MemoryManager> getMemoryManager() const { return m_memoryManager; }

    private:
        std::shared_ptr<MemoryManager> m_memoryManager;
        VkDevice m_device;
    };

} // namespace runtime
