#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include "../error_handling.h"
#include "descriptor_allocator.h"
#include "descriptor_layout_cache.h"

namespace runtime {

class DescriptorManager {
public:
    DescriptorManager(VkDevice device);
    ~DescriptorManager();

    // Prevent copying
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;

    // Allow moving
    DescriptorManager(DescriptorManager&&) noexcept = default;
    DescriptorManager& operator=(DescriptorManager&&) noexcept = default;

    void initialize();
    
    VkDescriptorSetLayout createDescriptorSetLayout(uint32_t set_index, 
                                                   const VkDescriptorSetLayoutCreateInfo* create_info);
    bool allocateDescriptorSet(VkDescriptorSet* descriptor_set, 
                              const VkDescriptorSetLayout* layout);
    
    void updateDescriptorSets(uint32_t write_count, 
                             const VkWriteDescriptorSet* writes);

private:
    VkDevice m_device;
    std::unique_ptr<class DescriptorAllocator> m_poolAllocator;
    std::unique_ptr<class DescriptorLayoutCache> m_layoutCache;
};

} // namespace runtime
