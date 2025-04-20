#ifndef DEVICE_H
#define DEVICE_H

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>


#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>


namespace runtime {

// Forward declarations
class DeviceFeatures;
class QueueManager;
class MemoryManager;
class Buffer;
class Program;
class DescriptorAllocator;
class DescriptorLayoutCache;
class CommandPoolManager;

class ThreadPool
{
  public:
    static std::shared_ptr<ThreadPool> create(size_t num_threads = std::thread::hardware_concurrency());
    ThreadPool(size_t num_threads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);

  private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    void workerThread();
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

class Device
{
  public:
    static std::shared_ptr<Device> create(std::shared_ptr<ThreadPool> pool, VkInstance instance, VkPhysicalDevice pd,
                                          const std::vector<uint32_t> &queue_counts);

     
    Device(std::shared_ptr<ThreadPool> pool, VkInstance instance, VkPhysicalDevice pd,
           const std::vector<uint32_t> &queue_counts);
    ~Device();
    // Buffer
    std::shared_ptr<Buffer> createWorkingBuffer(size_t size, bool is_dedicated = false);
    std::shared_ptr<Buffer> createSrcTransferBuffer(size_t size, bool is_dedicated = true);
    std::shared_ptr<Buffer> createDstTransferBuffer(size_t size, bool is_dedicated = true);
    void copyData(void *src, void *dst, size_t size);

    // Program
    std::shared_ptr<Program> createProgram(const std::vector<uint32_t> &shader, uint32_t dim_x=1, uint32_t dim_y=1,
                                           uint32_t dim_z=1);
    std::shared_ptr<CommandPoolManager> getComputePoolManager(size_t idx, VkQueueFlagBits flags);
    
    // Getters
    const DeviceFeatures& getDeviceFeatures() const { return *m_features; }
    VkDevice getDevice() const { return m_device; }
    void submit(const std::vector<std::shared_ptr<CommandPoolManager>> &cmdPools, uint32_t i = 0);
    
  private:
    // Initialize device
    bool initialize(VkInstance &instance, VkPhysicalDevice &pd,
                    const std::vector<uint32_t> &queue_counts);
    void cleanup();

    std::shared_ptr<Buffer> createBuffer(size_t size, VkBufferUsageFlags usage, VkFlags flags);
    // Device selection
    std::shared_ptr<ThreadPool> m_pool;
    std::unique_ptr<DeviceFeatures> m_features;
    std::shared_ptr<QueueManager> m_queue_manager;
    std::shared_ptr<MemoryManager> m_memory_manager;
    std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;
    std::shared_ptr<DescriptorLayoutCache> m_descriptorLayoutCache;

    VkDevice m_device{VK_NULL_HANDLE};
    VkPipelineCache m_pipeline_cache{VK_NULL_HANDLE}; // Add this line
    std::unordered_map<void*, std::shared_ptr<Buffer>> m_buffers;
};

} // namespace runtime

#endif // DEVICE_H