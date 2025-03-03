#include "vkd.h"
#include "error_handling.h"
#include <algorithm>

namespace runtime {

Device::Device(VkInstance& inst, VkPhysicalDevice& pd) 
    : m_features(pd)
    , m_queueManager(std::make_unique<QueueManager>(pd, m_device))
    , m_descriptorManager(std::make_unique<DescriptorManager>(m_device))
{
    initializeDevice(inst, pd);
}

Device::~Device() {
    cleanup();
}

Device::Device(Device&& other) noexcept 
    : m_device(other.m_device)
    , m_pipelineCache(other.m_pipelineCache)
    , m_allocator(other.m_allocator)
    , m_features(std::move(other.m_features))
    , m_queueManager(std::move(other.m_queueManager))
    , m_descriptorManager(std::move(other.m_descriptorManager))
    , m_execGraph(std::move(other.m_execGraph))
{
    other.m_device = VK_NULL_HANDLE;
    other.m_pipelineCache = VK_NULL_HANDLE;
    other.m_allocator = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        m_device = other.m_device;
        m_pipelineCache = other.m_pipelineCache;
        m_allocator = other.m_allocator;
        m_features = std::move(other.m_features);
        m_queueManager = std::move(other.m_queueManager);
        m_descriptorManager = std::move(other.m_descriptorManager);
        m_execGraph = std::move(other.m_execGraph);

        other.m_device = VK_NULL_HANDLE;
        other.m_pipelineCache = VK_NULL_HANDLE;
        other.m_allocator = VK_NULL_HANDLE;
    }
    return *this;
}

void Device::initializeDevice(VkInstance& inst, VkPhysicalDevice& pd) {
    // Create device with queue create infos
    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    auto queueCreateInfos = m_queueManager->getQueueCreateInfos();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    // Setup features chain
    const auto& features = m_features.getFeaturesChain();
    createInfo.pNext = &features.features2;

    check_result(vkCreateDevice(pd, &createInfo, nullptr, &m_device),
                "Failed to create logical device");

    volkLoadDevice(m_device);
    
    // Setup queues
    m_queueManager->setupQueues();

    // Create pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    check_result(vkCreatePipelineCache(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache),
                "Failed to create pipeline cache");

    // Setup VMA allocator
    setupVmaAllocator(inst, pd);
}

void Device::initializeAllocator(VkInstance &inst, VkPhysicalDevice &pd)
{
    VolkDeviceTable dt;
    volkLoadDeviceTable(&dt, m_device);

    VmaVulkanFunctions vmaFuncs{};
    vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    // ... setup other function pointers ...

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = pd;
    allocatorInfo.device = m_device;
    allocatorInfo.pVulkanFunctions = &vmaFuncs;
    allocatorInfo.instance = inst;
    allocatorInfo.preferredLargeHeapBlockSize = getMaxAllocationSize();
    allocatorInfo.vulkanApiVersion = volkGetInstanceVersion();
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
                         VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                         VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT |
                         VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;

    check_result(vmaCreateAllocator(&allocatorInfo, &m_allocator),
                "Failed to create VMA allocator");
}

void Device::cleanup() noexcept {
    if (m_device != VK_NULL_HANDLE) {
        // Cleanup execution graph nodes
        for (size_t i = 0; i < m_execGraph.size(); ++i) {
            auto node = m_execGraph.getNode(i);
            if (node->sType == vkrtType::BUFFER) {
                auto buffer = std::static_pointer_cast<vkrt_buffer>(node);
                vkDestroyBuffer(m_device, buffer->buffer, nullptr);
                vmaFreeMemory(m_allocator, buffer->allocation);
            }
            else if (node->sType == vkrtType::COMPUTE_PROGRAM) {
                auto program = std::static_pointer_cast<vkrt_compute_program>(node);
                // ... cleanup program resources ...
            }
        }

        m_queueManager->destroy();
        m_descriptorManager->destroy();

        if (m_allocator) {
            vmaDestroyAllocator(m_allocator);
            m_allocator = VK_NULL_HANDLE;
        }

        if (m_pipelineCache) {
            vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
            m_pipelineCache = VK_NULL_HANDLE;
        }
    }
}

void Device::update(uint32_t set_id, std::shared_ptr<vkrt_compute_program> program,
                std::vector<std::shared_ptr<vkrt_buffer>> &buffers) const
{
    for (uint32_t j = 0; j < program->n_set_bindings[set_id]; ++j)
    {
        VkDescriptorBufferInfo desc_buffer_info = {buffers[j]->buffer, 0, buffers[j]->allocation_info.size};
        program->writes[set_id][j].pBufferInfo = &desc_buffer_info;
    }

    vkUpdateDescriptorSets(m_device, program->n_set_bindings[set_id], program->writes[set_id], 0, nullptr);
}

} // namespace runtime
