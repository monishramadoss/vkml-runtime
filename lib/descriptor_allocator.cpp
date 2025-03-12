#include "descriptor_allocator.h"
#include <stdexcept>

namespace runtime {

DescriptorAllocator::DescriptorAllocator(const std::vector<VkDescriptorPoolSize>& poolSizes) 
    : m_poolSizes(poolSizes) {
}

DescriptorAllocator::~DescriptorAllocator() {
    // Make sure resources are properly cleaned up
    destroy();
}

DescriptorAllocator::DescriptorAllocator(DescriptorAllocator&& other) noexcept
    : m_device(other.m_device)
    , m_currentPool(other.m_currentPool)
    , m_usedPools(std::move(other.m_usedPools))
    , m_freePools(std::move(other.m_freePools))
    , m_poolSizes(std::move(other.m_poolSizes))
    , m_initialized(other.m_initialized) {
    
    // Reset moved-from object
    other.m_device = VK_NULL_HANDLE;
    other.m_currentPool = VK_NULL_HANDLE;
    other.m_initialized = false;
}

DescriptorAllocator& DescriptorAllocator::operator=(DescriptorAllocator&& other) noexcept {
    if (this != &other) {
        destroy();
        
        m_device = other.m_device;
        m_currentPool = other.m_currentPool;
        m_usedPools = std::move(other.m_usedPools);
        m_freePools = std::move(other.m_freePools);
        m_poolSizes = std::move(other.m_poolSizes);
        m_initialized = other.m_initialized;
        
        other.m_device = VK_NULL_HANDLE;
        other.m_currentPool = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

DescriptorAllocator& DescriptorAllocator::operator=(const DescriptorAllocator& other) {
    if (this != &other) {
        m_device = other.m_device;
        m_currentPool = other.m_currentPool;
        m_usedPools = other.m_usedPools;
        m_freePools = other.m_freePools;
        m_poolSizes = other.m_poolSizes;
        m_initialized = other.m_initialized;
    }
    return *this;
}

void DescriptorAllocator::initialize(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes) {
    if (m_initialized && m_device == device) {
        return; // Already initialized with this device
    }
    
    // Clean up existing resources if re-initializing
    if (m_initialized) {
        destroy();
    }
    
    m_device = device;
    m_poolSizes = poolSizes;
    m_initialized = true;
}

void DescriptorAllocator::resetPools() {
    if (!m_initialized) {
        return;
    }

    // Reset and move used pools to free list
    for (auto pool : m_usedPools) {
        vkResetDescriptorPool(m_device, pool, 0);
        m_freePools.push_back(pool);
    }
    m_usedPools.clear();
    m_currentPool = VK_NULL_HANDLE;
}

VkDescriptorPool DescriptorAllocator::createPool() {
    if (!m_initialized) {
        throw std::runtime_error("DescriptorAllocator not initialized");
    }

    // Calculate max sets based on descriptor counts
    constexpr uint32_t defaultMaxSets = 1000;
    constexpr float multiplier = 1.5f;  // Allow some overallocation for flexibility
    
    uint32_t maxSets = defaultMaxSets;
    if (!m_poolSizes.empty()) {
        maxSets = 0;
        for (const auto& size : m_poolSizes) {
            maxSets += size.descriptorCount;
        }
        maxSets = static_cast<uint32_t>(maxSets * multiplier);
    }
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;  // Allow individual free operations
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
    poolInfo.pPoolSizes = m_poolSizes.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &pool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
    
    return pool;
}

VkDescriptorPool DescriptorAllocator::grabPool() {
    if (!m_initialized) {
        throw std::runtime_error("DescriptorAllocator not initialized");
    }

    // Reuse an existing pool if available, otherwise create a new one
    if (!m_freePools.empty()) {
        VkDescriptorPool pool = m_freePools.back();
        m_freePools.pop_back();
        m_usedPools.push_back(pool);
        return pool;
    } else {
        VkDescriptorPool pool = createPool();
        m_usedPools.push_back(pool);
        return pool;
    }
}

bool DescriptorAllocator::allocate(VkDevice device, VkDescriptorSet* set, VkDescriptorSetLayout* layout) {
    if (!m_initialized) {
        throw std::runtime_error("DescriptorAllocator not initialized");
    }
    
    // Update device if provided
    if (device != VK_NULL_HANDLE) {
        m_device = device;
    }
    
    // Get a pool to allocate from
    if (m_currentPool == VK_NULL_HANDLE) {
        m_currentPool = grabPool();
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_currentPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layout;

    // Try to allocate from the current pool
    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, set);
    
    if (result == VK_SUCCESS) {
        return true;
    }
    
    // If allocation failed due to fragmentation or out of pool memory, try a fresh pool
    if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY) {
        m_currentPool = grabPool();
        allocInfo.descriptorPool = m_currentPool;
        
        result = vkAllocateDescriptorSets(m_device, &allocInfo, set);
        return (result == VK_SUCCESS);
    }
    
    // Failed for other reasons
    return false;
}

bool DescriptorAllocator::allocateMultiple(
    const std::vector<VkDescriptorSetLayout>& layouts, 
    std::vector<VkDescriptorSet>& sets) {
    
    if (!m_initialized) {
        throw std::runtime_error("DescriptorAllocator not initialized");
    }

    // Check for empty layouts
    if (layouts.empty()) {
        return true; // Nothing to allocate
    }

    // Resize output vector
    sets.resize(layouts.size(), VK_NULL_HANDLE);
    
    // Get a pool to allocate from
    if (m_currentPool == VK_NULL_HANDLE) {
        m_currentPool = grabPool();
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_currentPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    // Try to allocate from the current pool
    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, sets.data());
    
    if (result == VK_SUCCESS) {
        return true;
    }
    
    // If allocation failed due to fragmentation or out of pool memory, try a fresh pool
    if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY) {
        m_currentPool = grabPool();
        allocInfo.descriptorPool = m_currentPool;
        
        result = vkAllocateDescriptorSets(m_device, &allocInfo, sets.data());
        return (result == VK_SUCCESS);
    }
    
    // Failed for other reasons
    return false;
}

void DescriptorAllocator::destroy() {
    if (!m_initialized) {
        return;
    }

    // Clean up all descriptor pools
    for (auto pool : m_usedPools) {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    
    for (auto pool : m_freePools) {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    
    m_usedPools.clear();
    m_freePools.clear();
    m_currentPool = VK_NULL_HANDLE;
    m_initialized = false;
    m_device = VK_NULL_HANDLE;
}

} // namespace runtime
