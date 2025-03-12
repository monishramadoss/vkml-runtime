#ifndef DESCRIPTOR_ALLOCATOR_H
#define DESCRIPTOR_ALLOCATOR_H

#include <vulkan/vulkan.h>
#include <vector>

namespace runtime {

class DescriptorAllocator {
public:
    DescriptorAllocator(const std::vector<VkDescriptorPoolSize>& poolSizes = {});
    ~DescriptorAllocator();

    // Move semantics
    DescriptorAllocator(DescriptorAllocator&& other) noexcept;
    DescriptorAllocator& operator=(DescriptorAllocator&& other) noexcept;

    // Copy semantics
    DescriptorAllocator(const DescriptorAllocator& other) = default;
    DescriptorAllocator& operator=(const DescriptorAllocator& other);

    // Initialize with a specific device and pool sizes
    void initialize(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes = {});

    // Reset all used pools
    void resetPools();

    // Allocate a single descriptor set
    bool allocate(VkDevice device, VkDescriptorSet* set, VkDescriptorSetLayout* layout);
    
    // Allocate multiple descriptor sets at once
    bool allocateMultiple(
        const std::vector<VkDescriptorSetLayout>& layouts, 
        std::vector<VkDescriptorSet>& sets);

    // Clean up all resources
    void destroy();

private:
    // Helper methods
    VkDescriptorPool createPool();
    VkDescriptorPool grabPool();

    // Device handle
    VkDevice m_device{VK_NULL_HANDLE};
    
    // Descriptor pools
    VkDescriptorPool m_currentPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorPool> m_usedPools;
    std::vector<VkDescriptorPool> m_freePools;
    
    // Configuration
    std::vector<VkDescriptorPoolSize> m_poolSizes;
    bool m_initialized{false};
};

} // namespace runtime

#endif // DESCRIPTOR_ALLOCATOR_H