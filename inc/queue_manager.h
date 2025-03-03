#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <map>
#include "error_handling.h"

namespace runtime {

class QueueDispatcher;
struct ComputePacket;

class QueueManager {
public:
    QueueManager(VkPhysicalDevice pd, VkDevice device);
    ~QueueManager();

    // Prevent copying
    QueueManager(const QueueManager&) = delete;
    QueueManager& operator=(const QueueManager&) = delete;

    // Allow moving
    QueueManager(QueueManager&&) noexcept = default;
    QueueManager& operator=(QueueManager&&) noexcept = default;

    void initialize();
    
    // Queue submission and management
    void submit(const ComputePacket& packet);
    std::vector<uint32_t> getSparseQueueFamilyIndices() const;
    
    // Queue creation info for device creation
    std::vector<VkDeviceQueueCreateInfo> getQueueCreateInfo() const;

private:
    void setupQueues();
    size_t findQueueFamilies();

    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    std::vector<std::shared_ptr<QueueDispatcher>> m_queues;
    std::multimap<VkQueueFlags, uint32_t> m_queue_map;
    std::vector<float*> m_queue_priorities; // For cleanup
};

} // namespace runtime
