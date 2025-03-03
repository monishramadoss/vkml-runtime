#pragma once
#include "memory_manager.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vk_mem_alloc.h>
namespace runtime {
    class ImageAllocator {
    public:
        explicit ImageAllocator(std::shared_ptr<MemoryManager> memoryManager);
        ~ImageAllocator() = default;

        // Prevent copying
        ImageAllocator(const ImageAllocator&) = delete;
        ImageAllocator& operator=(const ImageAllocator&) = delete;

        struct AllocationResult {
            VkImage image;
            VmaAllocation allocation;
            VmaAllocationInfo allocationInfo;
        };

        AllocationResult allocateImage(const VkImageCreateInfo& imageInfo,
                                    const VmaAllocationCreateInfo& allocInfo);

        void freeImage(VkImage image, VmaAllocation allocation);

        std::shared_ptr<MemoryManager> getMemoryManager() const { return m_memoryManager; }

    private:
        std::shared_ptr<MemoryManager> m_memoryManager;
        VkDevice m_device;
    };

}  // namespace runtime