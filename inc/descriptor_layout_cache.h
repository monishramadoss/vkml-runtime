#ifndef DESCRIPTOR_LAYOUT_CACHE_H   
#define DESCRIPTOR_LAYOUT_CACHE_H

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>

namespace runtime {

class DescriptorLayoutCache {
public:
    DescriptorLayoutCache(VkDevice device = VK_NULL_HANDLE);
    ~DescriptorLayoutCache();
    
    void initialize(VkDevice device);
    void cleanup();

    // Prevent copying
    DescriptorLayoutCache(const DescriptorLayoutCache&) = delete;
    DescriptorLayoutCache& operator=(const DescriptorLayoutCache&) = delete;

    // Create a descriptor set layout from binding information
    VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    VkDescriptorSetLayout createDescriptorSetLayout(
        uint32_t setNumber, const VkDescriptorSetLayoutCreateInfo* createInfo) ;
        
private:
    // Device handle
    VkDevice m_device{VK_NULL_HANDLE};

    
    struct DescriptorLayoutInfo {
        uint32_t setNumber;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bool operator==(const DescriptorLayoutInfo& other) const;
        size_t hash() const;
    };

    // Helper for hashing layout bindings
    struct DescriptorLayoutHash {
        size_t operator()(const std::vector<DescriptorLayoutInfo>& bindings) const;
    };


    // Cache of created layouts
    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
    
};

} // namespace runtime

#endif // DESCRIPTOR_LAYOUT_CACHE_H