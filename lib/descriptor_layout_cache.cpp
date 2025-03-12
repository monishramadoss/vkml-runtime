#include "descriptor_layout_cache.h"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace runtime {

DescriptorLayoutCache::~DescriptorLayoutCache() {
    // Ensure cache is empty when destroyed
}

void DescriptorLayoutCache::initialize(VkDevice device) {
    m_device = device;
}

void DescriptorLayoutCache::cleanup() {
    if (!m_device) return;
    
    for (auto& pair : m_layoutCache) {
        vkDestroyDescriptorSetLayout(m_device, pair.second., nullptr);
    }
    m_layoutCache.clear();
}

VkDescriptorSetLayout DescriptorLayoutCache::createDescriptorSetLayout(
    uint32_t setNumber, const VkDescriptorSetLayoutCreateInfo* createInfo) {
    
    if (!m_device) {
        throw std::runtime_error("DescriptorLayoutCache not initialized with device");
    }

    // Build layout info object for cache lookup
    DescriptorLayoutInfo layoutInfo{};
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

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const {
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

size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const {
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

std::size_t DescriptorLayoutCache::DescriptorLayoutHash::operator()(const DescriptorLayoutInfo& k) const {
    return k.hash();
}

} // namespace runtime
