#include "device.h"
#include "error_handling.h"
#include "compute_program.h"
#include "memory_manager.h"
#include "queue_manager.h"

#include "device_features.h"
#include "buffer_allocator.h"
#include "queue_manager.h"

namespace runtime
{
    Device::Device(VkInstance& instance, VkPhysicalDevice& pd)
    {
        initialize(instance, pd);
    }

    Device::~Device() {
        cleanup();
    }

    Device::Device(Device&& other) noexcept 
        : m_device(other.m_device)
        , m_pipeline_cache(other.m_pipeline_cache)
        , m_physical_device(other.m_physical_device)
        , m_allocator(other.m_allocator)
        , m_features(std::move(other.m_features))
        , m_buffer_allocator(std::move(other.m_buffer_allocator))
        , m_memory_manager(std::move(other.m_memory_manager))
        , m_queue_manager(std::move(other.m_queue_manager))
    {
        other.m_device = VK_NULL_HANDLE;
        other.m_pipeline_cache = VK_NULL_HANDLE;
        other.m_physical_device = VK_NULL_HANDLE;
        other.m_allocator = VK_NULL_HANDLE;
    }

    Device& Device::operator=(Device&& other) noexcept {
        if (this != &other) {
            cleanup();
            
            m_device = other.m_device;
            m_pipeline_cache = other.m_pipeline_cache;
            m_physical_device = other.m_physical_device;
            m_allocator = other.m_allocator;
            m_features = std::move(other.m_features);
            m_buffer_allocator = std::move(other.m_buffer_allocator);
            m_memory_manager = std::move(other.m_memory_manager);
            m_queue_manager = std::move(other.m_queue_manager);

            other.m_device = VK_NULL_HANDLE;
            other.m_pipeline_cache = VK_NULL_HANDLE;
            other.m_physical_device = VK_NULL_HANDLE;
            other.m_allocator = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Device::initialize(VkInstance& instance, VkPhysicalDevice& pd)
    {
        m_physical_device = pd;
        
        // Create features first to use during device creation
        m_features = std::make_unique<DeviceFeatures>(pd);
        
        // Create queue manager to get queue create infos
        m_queue_manager = std::make_shared<QueueManager>(pd);
        
        // Create device with queue create infos
        VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        auto queueCreateInfos = m_queue_manager->getQueueCreateInfos();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        // Setup features chain
        const auto& features = m_features->getFeaturesChain();
        createInfo.pNext = &features;

        // Create device
        VkResult result = vkCreateDevice(pd, &createInfo, nullptr, &m_device);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        // Load device function pointers
        volkLoadDeviceTable(&m_vk_table, m_device);
        
        // Complete queue manager initialization with the device
        m_queue_manager->initialize(m_device);

        // Create pipeline cache
        VkPipelineCacheCreateInfo pipelineCacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        result = vkCreatePipelineCache(m_device, &pipelineCacheInfo, nullptr, &m_pipeline_cache);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline cache");
        }

        // Create memory allocator
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = pd;
        allocatorInfo.device = m_device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
        
        result = vmaCreateAllocator(&allocatorInfo, &m_allocator);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator");
        }
        
        // Create memory manager
        m_memory_manager = std::make_shared<MemoryManager>(m_allocator, m_device);
        
        // Create buffer allocator
        m_buffer_allocator = std::make_shared<BufferAllocator>(m_memory_manager);
    }

    void Device::cleanup() {
        if (m_device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_device);
            
            m_buffer_allocator.reset();
            m_memory_manager.reset();
            m_queue_manager.reset();
            
            if (m_allocator != VK_NULL_HANDLE) {
                vmaDestroyAllocator(m_allocator);
                m_allocator = VK_NULL_HANDLE;
            }
            
            if (m_pipeline_cache != VK_NULL_HANDLE) {
                vkDestroyPipelineCache(m_device, m_pipeline_cache, nullptr);
                m_pipeline_cache = VK_NULL_HANDLE;
            }
            
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
    }

    std::shared_ptr<vkrt_buffer> Device::createBuffer(const VkBufferCreateInfo* bufferInfo, VmaAllocationCreateInfo* allocInfo) {
        return m_buffer_allocator->allocateBuffer(bufferInfo, allocInfo);
    }

    std::shared_ptr<vkrt_compute_program> Device::createComputeProgram(const std::vector<uint32_t>& code) {
        // This needs to be implemented based on the ComputeProgram class
        // Most likely this should be delegated to a shader/program manager
        return m_memory_manager->createComputeProgram(code);
    }

    void Device::updateDescriptorSets(uint32_t set_id, 
                                     std::shared_ptr<vkrt_compute_program> program,
                                     const std::vector<std::shared_ptr<vkrt_buffer>>& buffers) {
        // Implement descriptor set updates - this would typically be handled by a descriptor manager
        // or as part of the compute program's functionality
    }

    void Device::mapBuffer(VmaAllocation alloc, void** data) const {
        vmaMapMemory(m_allocator, alloc, data);
    }

    void Device::unmapBuffer(VmaAllocation alloc) const {
        vmaUnmapMemory(m_allocator, alloc);
    }

    VkResult Device::copyMemoryToBuffer(VmaAllocation dst, const void* src, size_t size, size_t dst_offset) const {
        void* mapped_data;
        VkResult result = vmaMapMemory(m_allocator, dst, &mapped_data);
        if (result != VK_SUCCESS) {
            return result;
        }
        
        memcpy(static_cast<char*>(mapped_data) + dst_offset, src, size);
        vmaUnmapMemory(m_allocator, dst);
        
        return VK_SUCCESS;
    }

    VkResult Device::copyBufferToMemory(VmaAllocation src, void* dst, size_t size, size_t src_offset) const {
        void* mapped_data;
        VkResult result = vmaMapMemory(m_allocator, src, &mapped_data);
        if (result != VK_SUCCESS) {
            return result;
        }
        
        memcpy(dst, static_cast<char*>(mapped_data) + src_offset, size);
        vmaUnmapMemory(m_allocator, src);
        
        return VK_SUCCESS;
    }

    void Device::createEvent(const VkEventCreateInfo* info, VkEvent* event) const {
        VkResult result = vkCreateEvent(m_device, info, nullptr, event);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create event");
        }
    }

    void Device::destroyEvent(VkEvent event) const {
        vkDestroyEvent(m_device, event, nullptr);
    }

    void Device::destroy() {
        cleanup();
    }

} // namespace runtime