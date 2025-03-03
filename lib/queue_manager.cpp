#include "queue_manager.h"
#include "queue_dispatcher.h"
#include "compute_packet.h"
#include "error_handling.h"
#include <spdlog/spdlog.h>
#include <algorithm>    


namespace runtime {
    QueueManager::QueueManager(VkPhysicalDevice pd, VkDevice device)
        : m_physical_device(pd)
        , m_device(device)
    {
        initialize();
    }

    QueueManager::~QueueManager() {
        for (auto& queue : m_queues) {
            queue->destroy();
        }
    }

    void QueueManager::initialize() {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, queueFamilies.data());
        
        auto queueFamilyIndices = findQueueFamilies();
        setupQueues(queueFamilies);
    }

    void QueueManager::setupQueues(const std::vector<VkQueueFamilyProperties>& queueFamilies) {
        auto queueFamilyIndices = findQueueFamilies();
        for (auto idx : queueFamilyIndices) {
            if (idx < queueFamilies.size()) {
                m_queues.push_back(std::make_shared<QueueDispatcher>(idx, m_device));
            } else {
                spdlog::error("Invalid queue family index: {}", idx);
            }
        }
    }

    std::vector<uint32_t> QueueManager::findQueueFamilies() {
        std::vector<uint32_t> queueFamilyIndices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, queueFamilies.data());

        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueCount > 0) {
                queueFamilyIndices.push_back(i);
            }
            i++;
        }
        return queueFamilyIndices;
    }
    
    void QueueManager::submit(const ComputePacket& packet) {
        if (packet.queueFamilyIndex < m_queues.size()) {
            auto queue = m_queues[packet.queueFamilyIndex];
            queue->submit(packet);
        } else {
            std::cerr << "Invalid queue family index: " << packet.queueFamilyIndex << std::endl;
        }
        queue->submit(packet);
    }

    std::vector<uint32_t> QueueManager::getSparseQueueFamilyIndices() const {
        std::vector<uint32_t> sparseQueueFamilyIndices;
        for (const auto& queue : m_queues) {
            if (queue->isSparseBindingFamily()) {
                sparseQueueFamilyIndices.push_back(queue->get_idx());
            }
        }
        return sparseQueueFamilyIndices;
    }
   
    std::vector<VkDeviceQueueCreateInfo> QueueManager::getQueueCreateInfo() const {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (const auto& queue : m_queues) {
            VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queueCreateInfo.queueFamilyIndex = queue->get_idx();
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = queue->getQueuePriorities();
            queueCreateInfos.push_back(queueCreateInfo);
        }
        return queueCreateInfos;
    }
} // namespace runtime
