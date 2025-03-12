#ifndef QUEUE_DISPATCHER_H
#define QUEUE_DISPATCHER_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <mutex>
#include "error_handling.h"

namespace runtime {

struct ComputePacket; // Forward declaration

class QueueDispatcher {
public:
    QueueDispatcher(uint32_t queueFamilyIndex, VkDevice device, VkQueue queue);
    ~QueueDispatcher();

    // Prevent copying
    QueueDispatcher(const QueueDispatcher&) = delete;
    QueueDispatcher& operator=(const QueueDispatcher&) = delete;

    // Allow moving
    QueueDispatcher(QueueDispatcher&&) noexcept = default;
    QueueDispatcher& operator=(QueueDispatcher&&) noexcept = default;

    void submit(const ComputePacket& packet);
    void waitIdle();
    void destroy();

    bool supportsSparseBinding() const { return m_supportsSparseBinding; }
    uint32_t getQueueFamilyIndex() const { return m_queueFamilyIndex; }
    VkQueue getQueue() const { return m_queue; }

private:
    void createCommandPool();
    void createFence();

    VkDevice m_device{VK_NULL_HANDLE};
    VkQueue m_queue{VK_NULL_HANDLE};
    VkCommandPool m_commandPool{VK_NULL_HANDLE};
    VkFence m_fence{VK_NULL_HANDLE};
    uint32_t m_queueFamilyIndex{0};
    bool m_supportsSparseBinding{false};
    std::mutex m_queueMutex;
};

} // namespace runtime

#endif // QUEUE_DISPATCHER_H