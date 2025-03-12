#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <optional>
#include <functional>
#include "error_handling.h"
#include "logging.h"

namespace runtime {

// Forward declarations
class QueueDispatcher;
struct ComputePacket;

/**
 * @brief Manages Vulkan queue families and queue submission
 * 
 * Provides an abstraction over Vulkan queues to simplify queue family selection,
 * queue creation, and work submission.
 */
class QueueManager {
public:
    /**
     * @brief Queue capability flags for queue selection
     */
    enum class QueueCapability {
        GRAPHICS,
        COMPUTE,
        TRANSFER,
        SPARSE_BINDING,
        PRESENT
    };

    /**
     * @brief Construct a new Queue Manager
     * 
     * @param physicalDevice The physical device to query for queue families
     * @param device The logical device that owns the queues
     */
    QueueManager(VkPhysicalDevice physicalDevice, VkDevice device);
    
    /**
     * @brief Destroy the Queue Manager and free resources
     */
    ~QueueManager();

    // Prevent copying
    QueueManager(const QueueManager&) = delete;
    QueueManager& operator=(const QueueManager&) = delete;

    // Allow moving
    QueueManager(QueueManager&&) noexcept = default;
    QueueManager& operator=(QueueManager&&) noexcept = default;

    /**
     * @brief Initialize the queue manager
     * 
     * Must be called after construction and before using any other methods.
     * @throws std::runtime_error if initialization fails
     */
    void initialize();

    /**
     * @brief Submit work to an appropriate queue
     * 
     * @param packet The compute work packet to submit
     * @return VkResult Result of the submission operation
     * @throws std::runtime_error if no suitable queue is available
     */
    VkResult submit(const ComputePacket& packet);

    /**
     * @brief Get queue family indices that support sparse operations
     * 
     * @return std::vector<uint32_t> Queue family indices with sparse binding support
     */
    std::vector<uint32_t> getSparseQueueFamilyIndices() const;
    
    /**
     * @brief Get device queue create info for device creation
     * 
     * Call this before device creation to get the queue creation info needed.
     * 
     * @return std::vector<VkDeviceQueueCreateInfo> Queue creation info structures
     */
    std::vector<VkDeviceQueueCreateInfo> getQueueCreateInfo() const;
    
    /**
     * @brief Find a queue with specified capabilities
     * 
     * @param capabilities Required capabilities for the queue
     * @return std::optional<uint32_t> Queue family index if found, empty otherwise
     */
    std::optional<uint32_t> findQueueFamily(std::vector<QueueCapability> capabilities) const;

private:
    void setupQueues();
    size_t discoverQueueFamilies();
    void createQueueDispatchers();
    
    struct QueueFamilyInfo {
        uint32_t index;
        VkQueueFlags flags;
        uint32_t queueCount;
        std::unique_ptr<float[]> priorities;
    };

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    std::vector<std::shared_ptr<QueueDispatcher>> m_queueDispatchers;
    std::multimap<VkQueueFlags, uint32_t> m_queueMap;
    std::vector<QueueFamilyInfo> m_queueFamilies;
    bool m_initialized{false};
};

} // namespace runtime

#endif // QUEUE_MANAGER_H