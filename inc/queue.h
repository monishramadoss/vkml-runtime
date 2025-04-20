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
    
    void submitCompute(VkPipeline pipeline, VkPipelineLayout layout, uint32_t n_sets, const VkDescriptorSet *pDescriptors,
                VkPipelineBindPoint bindPoint, uint32_t dim_x, uint32_t dim_y, uint32_t dim_z);
    

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

    std::mutex m;
    std::condition_variable cv;
    bool ready;
    VkDevice m_device;
    VkQueueFamilyProperties m_queueFamilyProperties;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_primaryCommandBuffer;
    std::bitset<SECONDARY_BUFFER> used_buffers;
    std::vector<VkCommandBuffer> m_secondaryCommandBuffers;
    std::shared_ptr<ThreadPool> m_threadPool;
};

class QueueManager
{
  public:
    static std::shared_ptr<QueueManager> create(VkPhysicalDevice &pDevice);

    QueueManager(VkPhysicalDevice& pDevice);
    ~QueueManager();
    
    std::vector<VkDeviceQueueCreateInfo> getQueueCreateInfos() const
    {
        return m_queueCreateInfos;
    }

    std::shared_ptr<CommandPoolManager> getComputeQueue(uint32_t i = 0);
    VkQueue getSparseQueue(uint32_t i = 0) const;
    void start(std::shared_ptr<ThreadPool> , VkPhysicalDevice &pDevice, VkDevice &device);
    void run(uint32_t i = 0);

  private:
    static void submitQueue(VkFence fence, VkQueue queue, uint32_t n_cmds, const VkCommandBuffer *pCmdBuffers,
        const VkPipelineStageFlags *pWaitDstStageMask = nullptr,
        uint32_t n_wait_semaphores = 0, 
        const VkSemaphore* wait_semaphore = nullptr,
        uint32_t n_signal_semaphores = 0, 
        const VkSemaphore* signal_semaphores = nullptr);
    // Initialize queue families
    bool initialize(VkPhysicalDevice& pDevice);
    void cleanup();
    std::vector<VkDeviceQueueCreateInfo> m_queueCreateInfos;
    std::vector<VkQueueFamilyProperties> queueFamilies;
    std::vector<std::shared_ptr<CommandPoolManager>> m_commandPoolManagers;   
    std::vector<VkQueue> m_queues;
    std::vector<VkFence> m_fences;
    VkDevice m_device{VK_NULL_HANDLE};
    std::shared_ptr<ThreadPool> m_threadPool;
};
} // namespace runtim



#endif 