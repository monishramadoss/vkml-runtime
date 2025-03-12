#ifndef DESCRIPTOR_LAYOUT_CACHE_H
#define DESCRIPTOR_LAYOUT_CACHE_H

#include <vulkan/vulkan.h>
#include <volk.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>

namespace runtime {

/**
 * @brief Cache for Vulkan descriptor set layouts to avoid redundant creation
 * 
 * This class maintains a cache of descriptor set layouts to avoid the overhead
 * of creating duplicate layouts, which is a common occurrence in Vulkan applications.
 */
class DescriptorLayoutCache {
public:
    DescriptorLayoutCache() = default;
    
    ~DescriptorLayoutCache() {
        // Ensure cache is empty when destroyed
    }

    /**
     * @brief Initialize the cache with the device handle
     * 
     * @param device The logical device to use for layout creation
     */
    void initialize(VkDevice device) {
        m_device = device;
    }

    /**
     * @brief Destroy all cached descriptor set layouts
     */
    void destroy() {
        if (!m_device) return;
        
        for (auto& pair : m_layoutCache) {
            vkDestroyDescriptorSetLayout(m_device, pair.second, nullptr);
        }
        m_layoutCache.clear();
    }

    /**
     * @brief Create or retrieve a descriptor set layout from cache
     * 
     * @param setNumber Identifier for the set (pipeline slot)
     * @param createInfo Layout creation configuration
     * @return VkDescriptorSetLayout The created or cached layout
     * @throws std::runtime_error if layout creation fails
     */
    VkDescriptorSetLayout createDescriptorLayout(uint32_t setNumber, 
                                                VkDescriptorSetLayoutCreateInfo* createInfo) {
        if (!m_device) {
            throw std::runtime_error("DescriptorLayoutCache not initialized with device");
        }

        // Build layout info object for cache lookup
        DescriptorLayoutInfo layoutInfo;
        layoutInfo.setNumber = setNumber;
        layoutInfo.bindings.reserve(createInfo->bindingCount);

        // Copy and ensure bindings are sorted
        for (uint32_t i = 0; i < createInfo->bindingCount; i++) {
            layoutInfo.bindings.push_back(createInfo->pBindings[i]);
        }
        
        // Sort bindings by binding number for consistent lookup
        std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(),
            [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b) { 
                return a.binding < b.binding; 
            });

        // Check if we already have this layout cached
        auto it = m_layoutCache.find(layoutInfo);
        if (it != m_layoutCache.end()) {
            return it->second;
        }

        // Create new layout
        VkDescriptorSetLayout layout;
        VkResult result = vkCreateDescriptorSetLayout(m_device, createInfo, nullptr, &layout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }
        
        // Cache and return the new layout
        m_layoutCache[layoutInfo] = layout;
        return layout;
    }

private:
    /**
     * @brief Internal representation of descriptor set layout for caching
     */
    struct DescriptorLayoutInfo {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        uint32_t setNumber = UINT32_MAX;

        bool operator==(const DescriptorLayoutInfo& other) const {
            if (other.setNumber != setNumber || other.bindings.size() != bindings.size()) {
                return false;
            }

            // Compare all binding properties
            for (size_t i = 0; i < bindings.size(); i++) {
                const auto& a = bindings[i];
                const auto& b = other.bindings[i];
                
                if (a.binding != b.binding || 
                    a.descriptorType != b.descriptorType ||
                    a.descriptorCount != b.descriptorCount ||
                    a.stageFlags != b.stageFlags) {
                    return false;
                }
            }
            return true;
        }

        size_t hash() const {
            size_t result = std::hash<uint32_t>()(setNumber);
            
            // Combine hashes of all bindings
            for (const auto& binding : bindings) {
                // Pack binding properties into a hash
                size_t bindingHash = binding.binding;
                bindingHash ^= static_cast<size_t>(binding.descriptorType) << 8;
                bindingHash ^= static_cast<size_t>(binding.descriptorCount) << 16;
                bindingHash ^= static_cast<size_t>(binding.stageFlags) << 24;
                
                // Combine with result using a prime number
                result ^= bindingHash + 0x9e3779b9 + (result << 6) + (result >> 2);
            }
            return result;
        }
    };

    /**
     * @brief Hash functor for DescriptorLayoutInfo
     */
    struct DescriptorLayoutHash {
        std::size_t operator()(const DescriptorLayoutInfo& k) const {
            return k.hash();
        }
    };

    VkDevice m_device = VK_NULL_HANDLE;
    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
};

} // namespace runtime

#endif // DESCRIPTOR_LAYOUT_CACHE_H