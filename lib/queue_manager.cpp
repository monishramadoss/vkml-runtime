#include "queue_manager.h"
#include "queue_dispatcher.h"
#include "compute_packet.h"
#include "error_handling.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <stdexcept>

namespace runtime {
    QueueManager::QueueManager(VkPhysicalDevice pd)
        : m_physical_device(pd)
        , m_device(VK_NULL_HANDLE)
    {
        // Device will be set during initialize()
    }

    QueueManager::~QueueManager() {
        for (auto& queue : m_queues) {
            queue.reset();
        }
        m_queues.clear();
    }

    void QueueManager::initialize(VkDevice device) {
        m_device = device;
        
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, queueFamilies.data());
        
        setupQueues(queueFamilies);
    }

    std::vector<VkDeviceQueueCreateInfo> QueueManager::getQueueCreateInfos() const {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::vector<uint32_t> uniqueFamilies;
        
        // Find queue families with compute support
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, queueFamilies.data());
        
        // Find compute and transfer capable queue families
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                uniqueFamilies.push_back(i);
            }
        }
        
        // Standard queue priorities
        static const float queuePriority = 1.0f;
        
        // Create one queue from each family
        for (uint32_t queueFamily : uniqueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        return queueCreateInfos;
    }

    void QueueManager::setupQueues(const std::vector<VkQueueFamilyProperties>& queueFamilies) {
        // Create dispatchers for each queue family that supports compute
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                VkQueue queue = VK_NULL_HANDLE;
                vkGetDeviceQueue(m_device, i, 0, &queue);
                
                if (queue != VK_NULL_HANDLE) {
                    auto dispatcher = std::make_shared<QueueDispatcher>(i, m_device, queue);
                    m_queues.push_back(dispatcher);
                }
            }
        }
        
        if (m_queues.empty()) {
            throw std::runtime_error("Failed to find any compute queues");
        }
    }

    std::vector<uint32_t> QueueManager::findQueueFamilies() {
        std::vector<uint32_t> queueFamilyIndices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, queueFamilies.data());

        // Identify queue families with compute support
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueCount > 0 && 
                queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queueFamilyIndices.push_back(i);
            }
        }
        
        return queueFamilyIndices;
    }
    
    void QueueManager::submit(const ComputePacket& packet) {
        // Find an appropriate queue for this packet
        if (m_queues.empty()) {
            throw std::runtime_error("No compute queues available");
        }
        
        // Get queue family index from packet or use the first available
        uint32_t queueIndex = 0;
        if (packet.queueFamilyIndex < m_queues.size()) {
            queueIndex = packet.queueFamilyIndex;
        }
        
        // Submit the compute packet
        m_queues[queueIndex]->submit(packet);
    }

    std::vector<uint32_t> QueueManager::getSparseQueueFamilyIndices() const {
        std::vector<uint32_t> sparseQueueFamilyIndices;
        
        for (const auto& queue : m_queues) {
            if (queue->supportsSparseBinding()) {
                sparseQueueFamilyIndices.push_back(queue->getQueueFamilyIndex());
            }
        }
        
        return sparseQueueFamilyIndices;
    }
   
    QueueDispatcher* QueueManager::getQueueDispatcher(uint32_t familyIndex) {
        for (auto& queue : m_queues) {
            if (queue->getQueueFamilyIndex() == familyIndex) {
                return queue.get();
            }
        }
        return nullptr;
    }
} // namespace runtime
