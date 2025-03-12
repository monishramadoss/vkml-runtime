// This file is part of the `runtime` library of Vulkan-Hpp-Samples.
#ifndef DESCRIPTOR_MANAGER_H
#define DESCRIPTOR_MANAGER_H
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace runtime {

// Forward declarations
class DescriptorAllocator;
class DescriptorLayoutCache;
class Buffer;
class ComputeProgram;

/**
 * @brief Manages descriptor layouts and allocations with optimized caching
 */
class DescriptorManager {
public:
    /**
     * @brief Constructs a descriptor manager
     */
    DescriptorManager(VkDevice device = VK_NULL_HANDLE);
    
    /**
     * @brief Destructor
     */
    ~DescriptorManager();
    
    /**
     * @brief Get default pool sizes for typical compute workloads
     */
    static std::vector<VkDescriptorPoolSize> getDefaultPoolSizes();
    
    /**
     * @brief Initializes the descriptor manager
     * @param device Vulkan device
     * @param poolSizes Vector of descriptor pool sizes to allocate
     */
    void initialize(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes);
    
    /**
     * @brief Creates or retrieves a cached descriptor set layout
     */
    VkDescriptorSetLayout createDescriptorSetLayout(uint32_t setNumber, 
                                                  const VkDescriptorSetLayoutCreateInfo* createInfo);
    
    /**
     * @brief Allocates a descriptor set using a layout
     */
    bool allocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet* set);
    
    /**
     * @brief Allocates multiple descriptor sets at once
     */
    bool allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& layouts, 
                               std::vector<VkDescriptorSet>& sets);
    
    /**
     * @brief Updates descriptor sets with the provided write operations
     */
    void updateDescriptorSets(uint32_t writeCount, const VkWriteDescriptorSet* writes);
    
    /**
     * @brief Updates buffer descriptors for a compute program
     */
    void updateBufferDescriptors(uint32_t setId, 
                                std::shared_ptr<ComputeProgram> program,
                                const std::vector<std::shared_ptr<Buffer>>& buffers);
    
    /**
     * @brief Releases a descriptor set layout (for reuse or cleanup)
     */
    void releaseDescriptorSetLayout(uint32_t setNumber, VkDescriptorSetLayout layout);

    /**
     * @brief Releases a descriptor set (for reuse)
     */
    void releaseDescriptorSet(VkDescriptorSet set);
    
    /**
     * @brief Resets all used descriptor pools
     */
    void resetPools();

    /**
     * @brief Releases all resources
     */
    void destroy();

private:
    VkDevice m_device = VK_NULL_HANDLE;
    std::unique_ptr<DescriptorLayoutCache> m_layoutCache;
    std::unique_ptr<DescriptorAllocator> m_allocator;
    bool m_initialized = false;
};

} // namespace runtime


#endif // DESCRIPTOR_MANAGER_H