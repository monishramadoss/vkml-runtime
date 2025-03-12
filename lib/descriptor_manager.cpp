#include "descriptor_manager.h"
#include "descriptor_allocator.h"
#include "descriptor_layout_cache.h"
#include <stdexcept>

namespace runtime {

DescriptorManager::DescriptorManager(VkDevice device) : m_device(device) {
    if (device != VK_NULL_HANDLE) {
        initialize(device, getDefaultPoolSizes());
    }
}

DescriptorManager::~DescriptorManager() {
    destroy();
}

std::vector<VkDescriptorPoolSize> DescriptorManager::getDefaultPoolSizes() {
    // Default sizes for common descriptor types
    return {
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16 }
    };
}

void DescriptorManager::initialize(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes) {
    m_device = device;
    
    if (!m_layoutCache) {
        m_layoutCache = std::make_unique<DescriptorLayoutCache>();
        m_layoutCache->initialize(device);
    }
    
    if (!m_allocator) {
        m_allocator = std::make_unique<DescriptorAllocator>();
        m_allocator->initialize(device, poolSizes);
    }
    
    m_initialized = true;
}

VkDescriptorSetLayout DescriptorManager::createDescriptorSetLayout(
    uint32_t setNumber, 
    const VkDescriptorSetLayoutCreateInfo* createInfo) {
    
    if (!m_initialized || !m_layoutCache) {
        throw std::runtime_error("DescriptorManager not initialized");
    }
    
    return m_layoutCache->createDescriptorLayout(setNumber, const_cast<VkDescriptorSetLayoutCreateInfo*>(createInfo));
}

bool DescriptorManager::allocateDescriptorSet(
    VkDescriptorSetLayout layout, 
    VkDescriptorSet* set) {
    
    if (!m_initialized || !m_allocator) {
        throw std::runtime_error("DescriptorManager not initialized");
    }
    
    return m_allocator->allocate(m_device, set, &layout);
}

bool DescriptorManager::allocateDescriptorSets(
    const std::vector<VkDescriptorSetLayout>& layouts, 
    std::vector<VkDescriptorSet>& sets) {
    
    if (!m_initialized || !m_allocator) {
        throw std::runtime_error("DescriptorManager not initialized");
    }
    
    return m_allocator->allocateMultiple(layouts, sets);
}

void DescriptorManager::updateDescriptorSets(
    uint32_t writeCount, 
    const VkWriteDescriptorSet* writes) {
    
    if (!m_initialized) {
        throw std::runtime_error("DescriptorManager not initialized");
    }
    
    vkUpdateDescriptorSets(m_device, writeCount, writes, 0, nullptr);
}

void DescriptorManager::updateBufferDescriptors(
    uint32_t setId, 
    std::shared_ptr<vkrt_compute_program> program,
    const std::vector<std::shared_ptr<vkrt_buffer>>& buffers) {
    
    if (!m_initialized) {
        throw std::runtime_error("DescriptorManager not initialized");
    }
    
    if (!program) {
        throw std::runtime_error("Invalid compute program");
    }
    
    // Get descriptor set for the specified ID
    VkDescriptorSet targetSet = program->getDescriptorSet(setId);
    if (targetSet == VK_NULL_HANDLE) {
        throw std::runtime_error("Invalid descriptor set for set ID: " + std::to_string(setId));
    }
    
    // Create buffer infos and write descriptor sets
    std::vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());
    std::vector<VkWriteDescriptorSet> writes(buffers.size());
    
    for (size_t i = 0; i < buffers.size(); ++i) {
        auto& buffer = buffers[i];
        
        if (!buffer) {
            throw std::runtime_error("Invalid buffer at index " + std::to_string(i));
        }
        
        // Create buffer info
        bufferInfos[i] = {};
        bufferInfos[i].buffer = buffer->getHandle();
        bufferInfos[i].offset = 0;
        bufferInfos[i].range = buffer->getSize();
        
        // Create write descriptor set
        writes[i] = {};
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = targetSet;
        writes[i].dstBinding = static_cast<uint32_t>(i);
        writes[i].dstArrayElement = 0;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = buffer->isUniform() ? 
                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : 
                                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &bufferInfos[i];
    }
    
    // Update descriptor sets
    if (!writes.empty()) {
        updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data());
    }
}

void DescriptorManager::releaseDescriptorSetLayout(uint32_t setNumber, VkDescriptorSetLayout layout) {
    // This is a no-op since layouts are cached and managed by the DescriptorLayoutCache
    // They will be automatically cleaned up when the cache is destroyed
}

void DescriptorManager::releaseDescriptorSet(VkDescriptorSet set) {
    // Descriptor sets are allocated from pools and will be automatically freed
    // when the pool is reset or destroyed
}

void DescriptorManager::resetPools() {
    if (m_initialized && m_allocator) {
        m_allocator->resetPools();
    }
}

void DescriptorManager::destroy() {
    if (m_layoutCache) {
        m_layoutCache->destroy();
        m_layoutCache.reset();
    }
    
    if (m_allocator) {
        m_allocator->destroy();
        m_allocator.reset();
    }
    
    m_initialized = false;
}

} // namespace runtime