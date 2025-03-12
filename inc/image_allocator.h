#ifndef VKML_IMAGE_ALLOCATOR_H
#define VKML_IMAGE_ALLOCATOR_H

#include "memory_manager.h"
#include "error_handling.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vk_mem_alloc.h>

namespace runtime {

/**
 * @brief Manages image allocation and memory management using VMA
 * 
 * Provides convenient allocation of Vulkan images with automatic memory management,
 * leveraging the Vulkan Memory Allocator library for optimal memory placement.
 */
class ImageAllocator {
public:
    /**
     * @brief Create an image allocator with the given memory manager
     * @param memoryManager Shared memory manager for this allocator
     * @throws VulkanError if initialization fails
     */
    explicit ImageAllocator(std::shared_ptr<MemoryManager> memoryManager);
    
    /**
     * @brief Clean up resources
     */
    ~ImageAllocator();

    // Prevent copying
    ImageAllocator(const ImageAllocator&) = delete;
    ImageAllocator& operator=(const ImageAllocator&) = delete;

    // Allow moving
    ImageAllocator(ImageAllocator&&) noexcept = default;
    ImageAllocator& operator=(ImageAllocator&&) noexcept = default;

    /**
     * @brief Result of an image allocation operation
     */
    struct AllocationResult {
        VkImage image;              ///< Vulkan image handle
        VmaAllocation allocation;   ///< VMA allocation handle
        VmaAllocationInfo allocationInfo; ///< Details about the allocation
    };

    /**
     * @brief Allocate a new image with associated memory
     * 
     * @param imageInfo Vulkan image creation parameters
     * @param allocInfo VMA allocation parameters
     * @return Allocation result containing handles
     * @throws VulkanError on allocation failure
     */
    AllocationResult allocateImage(const VkImageCreateInfo& imageInfo,
                                const VmaAllocationCreateInfo& allocInfo);

    /**
     * @brief Free an allocated image
     * 
     * @param image Image handle to free
     * @param allocation VMA allocation to release
     */
    void freeImage(VkImage image, VmaAllocation allocation);

    /**
     * @brief Access the underlying memory manager
     * @return Shared pointer to the memory manager
     */
    [[nodiscard]] std::shared_ptr<MemoryManager> getMemoryManager() const noexcept { 
        return m_memoryManager; 
    }

private:
    std::shared_ptr<MemoryManager> m_memoryManager;
    VkDevice m_device{VK_NULL_HANDLE};
};

}  // namespace runtime

#endif // VKML_IMAGE_ALLOCATOR_H