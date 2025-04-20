#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <bitset>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <unordered_map>

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif // VOLK_HH

#define SECONDARY_BUFFER 8

namespace runtime
{
class Device;
class ThreadPool;

class DescriptorLayoutCache
{
  public:
    static std::shared_ptr<DescriptorLayoutCache> create(VkDevice device);
    
    DescriptorLayoutCache(VkDevice device);
    ~DescriptorLayoutCache();

    VkDescriptorSetLayout getDescriptorSetLayout(uint32_t set, VkDescriptorSetLayoutCreateInfo *createInfo);


  private:
    void initialize(VkDevice device);
    void cleanup();

    VkDevice m_device;
    struct DescriptorLayoutInfo
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        uint32_t setNumber = UINT32_MAX;

        bool operator==(const DescriptorLayoutInfo &other) const
        {
            if (other.setNumber != setNumber || other.bindings.size() != bindings.size())
            {
                return false;
            }

            // Compare all binding properties
            for (size_t i = 0; i < bindings.size(); i++)
            {
                const auto &a = bindings[i];
                const auto &b = other.bindings[i];

                if (a.binding != b.binding || a.descriptorType != b.descriptorType ||
                    a.descriptorCount != b.descriptorCount || a.stageFlags != b.stageFlags)
                {
                    return false;
                }
            }
            return true;
        }

        size_t hash() const
        {
            size_t result = std::hash<uint32_t>()(setNumber);

            // Combine hashes of all bindings
            for (const auto &binding : bindings)
            {
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
    };

    /**
     * @brief Hash functor for DescriptorLayoutInfo
     */
    struct DescriptorLayoutHash
    {
        std::size_t operator()(const DescriptorLayoutInfo &k) const
        {
            return k.hash();
        }
    };

    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
};

class DescriptorAllocator
{
    
  public:
    static std::shared_ptr<DescriptorAllocator> create(VkDevice device, const std::vector<VkDescriptorPoolSize> &pool_sizes);
    DescriptorAllocator(VkDevice device, const std::vector<VkDescriptorPoolSize> &pool_sizes);
    ~DescriptorAllocator();
  
    void resetPools();
    bool allocate(size_t n_sets, VkDescriptorSet *set, VkDescriptorSetLayout *layout);
    
  private:
    void initialize(VkDevice device, const std::vector<VkDescriptorPoolSize>& pool_sizes);
    void cleanup();
    VkDescriptorPool currentPool{VK_NULL_HANDLE};
    std::vector<VkDescriptorPool> usedPools;
    std::vector<VkDescriptorPool> freePools;
    std::vector<VkDescriptorPoolSize> pool_size;
    VkDevice m_device;

    VkDescriptorPool createPool();
    VkDescriptorPool grabPool();
};

class Program;
struct QueueData
{
    VkQueue queue;
    VkFence fence;
    uint32_t queueFamilyIndex;
    uint32_t queueIndex;
    std::promise<int> promise;
};

class CommandPoolManager
{
  public:
    static std::shared_ptr<CommandPoolManager> create(std::shared_ptr<ThreadPool> pool, VkDevice device, uint32_t queueIndex,
                                                      VkQueueFamilyProperties m_queueFamilyProperties);
    CommandPoolManager(std::shared_ptr<ThreadPool> pool, VkDevice device, uint32_t queueIndex,
                       VkQueueFamilyProperties m_queueFamilyProperties);
    ~CommandPoolManager();
    
    VkCommandPool getCommandPool();
    VkCommandBuffer getPrimaryCommandBuffer();
    std::vector<VkCommandBuffer> getSecondaryCommandBuffer();
    VkQueueFamilyProperties getQueueFamilyProperties() const;
    uint32_t getQueueFamilyIndex() const;
    void submitCompute(VkPipeline pipeline, VkPipelineLayout layout, uint32_t n_sets, const VkDescriptorSet *pDescriptors,
                VkPipelineBindPoint bindPoint, uint32_t dim_x, uint32_t dim_y, uint32_t dim_z);
    bool is_ready();
    void set_future(const std::shared_future<int> &fut);
    void wait();

    // New methods
    void setPromise(std::shared_ptr<std::promise<int>> promise);
    bool waitForReady(std::chrono::duration<int64_t> timeout);

  private:
    void initialize(VkDevice device, uint32_t queueIndex);
    void cleanup();
    static void secondaryCommandBufferRecord(VkCommandBuffer commandBuffer, VkPipeline pipeline,
                                             VkPipelineLayout layout, uint32_t n_sets,
                                             const VkDescriptorSet *pDescriptors, VkPipelineBindPoint bindPoint,
                                             uint32_t dim_x, uint32_t dim_y, uint32_t dim_z);
    static void primaryCommandBufferRecord(VkCommandBuffer commandBuffer, uint32_t n_cmds,
                                           const VkCommandBuffer *pCmdBuffers);
    size_t findAvailableCommandBuffer();
  

    // Rename mutex and condition variable for clarity
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool ready;
    VkDevice m_device;
    VkQueueFamilyProperties m_queueFamilyProperties;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_primaryCommandBuffer;
    uint32_t m_queueFamilyIndex;
    std::bitset<SECONDARY_BUFFER> used_buffers;
    std::vector<VkCommandBuffer> m_secondaryCommandBuffers;
    std::shared_ptr<ThreadPool> m_threadPool;    std::shared_future<int> m_fut;
    bool m_hasFuture = false;  // Flag to track if future has been set

    // Add shared promise for coordination
    std::shared_ptr<std::promise<int>> m_promise;
};

class QueueManager
{
  public:
    static std::shared_ptr<QueueManager> create(VkPhysicalDevice &pDevice, const std::vector<uint32_t> &queue_count);

    QueueManager(VkPhysicalDevice &pDevice, const std::vector<uint32_t> &queue_count);
    ~QueueManager();
    
    std::vector<VkDeviceQueueCreateInfo> getQueueCreateInfos() const
    {
        return m_queueCreateInfos;
    }

    VkQueue getSparseQueue(uint32_t i = 0) const;
    void start(std::shared_ptr<ThreadPool> , VkPhysicalDevice &pDevice, VkDevice &device);
    void run(const std::vector<std::shared_ptr<CommandPoolManager>> &cmdPoolManagers,
                                uint32_t i = 0);
    uint32_t getQueueFamilyIndex(VkQueueFlagBits queue_flags) const;
    VkQueueFamilyProperties getQueueFamilyProperties(uint32_t i) const;

  private:
    static void submitQueue(VkFence fence, VkQueue queue, uint32_t n_cmds, const VkCommandBuffer *pCmdBuffers,
        const VkPipelineStageFlags *pWaitDstStageMask = nullptr,
        uint32_t n_wait_semaphores = 0, 
        const VkSemaphore* wait_semaphore = nullptr,
        uint32_t n_signal_semaphores = 0, 
        const VkSemaphore* signal_semaphores = nullptr);
    // Initialize queue families
    bool initialize(VkPhysicalDevice& pDevice, const std::vector<uint32_t> &queue_count);
    void cleanup();
    std::vector<VkDeviceQueueCreateInfo> m_queueCreateInfos;
    std::vector<VkQueueFamilyProperties> m_queueFamilies; 

    std::mutex m_QueueM;
    std::condition_variable m_QueueC;
    std::vector<std::shared_ptr<QueueData>> m_queueData;
    std::queue<uint32_t> m_queueFlags;

    std::mutex m_workPoolM;
    std::condition_variable m_workPoolC;
    std::queue<std::pair<std::vector<std::shared_ptr<CommandPoolManager>>, 
                         std::shared_ptr<std::promise<int>>>> m_cmdPoolQueue;

    //std::unordered_multimap<VkQueueFlagBits, QueuePacket> m_queuePackets;
    VkDevice m_device{VK_NULL_HANDLE};
    std::shared_ptr<ThreadPool> m_threadPool;
};
} // namespace runtim



#endif 