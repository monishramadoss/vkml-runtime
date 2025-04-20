#include "device.h"

#include "error_handling.h"
#include "device_features.h"
#include "queue.h"
#include "storage.h"
#include "program.h"

#ifndef VOLK_HH
#define VOLK_HH
#include <volk.h>
#endif // VOLK_HH


namespace runtime
{
        
    std::shared_ptr<ThreadPool> ThreadPool::create(size_t num_threads)
    {
        return std::make_shared<ThreadPool>(num_threads);
    }

    ThreadPool::ThreadPool(size_t num_threads) : stop(false)
    {
        workers.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back(&ThreadPool::workerThread, this);
        }
    }

    ThreadPool::~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
        {
            worker.join();
        }
    }

    void ThreadPool::enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }

    void ThreadPool::workerThread()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this] { return stop || !tasks.empty(); });
                if (stop && tasks.empty())
                {
                    return;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }

    std::shared_ptr<Device> Device::create(std::shared_ptr<ThreadPool> pool, VkInstance instance, VkPhysicalDevice pd,
                                           const std::vector<uint32_t> &queue_counts)
    {
 
        return std::make_shared<Device>(pool, instance, pd, queue_counts);
    }
    
    Device::Device(std::shared_ptr<ThreadPool> pool, VkInstance instance, VkPhysicalDevice pd,
                   const std::vector<uint32_t> &queue_counts)
        : m_pool(pool), m_device(VK_NULL_HANDLE), m_pipeline_cache(VK_NULL_HANDLE), m_memory_manager(nullptr),
          m_queue_manager(nullptr), m_descriptorLayoutCache(nullptr), m_descriptorAllocator(nullptr)
    {
        initialize(instance, pd, queue_counts);
    }

    Device::~Device()
    {
        cleanup();
    }
       
    std::shared_ptr<Buffer> Device::createBuffer(size_t size, VkBufferUsageFlags usage, VkFlags flags)
    {
        if (!size)
            LOG_ERROR("Invalid buffer size");
        if (m_memory_manager && size < m_features->getMaxAllocationSize())
        {
            return Buffer::create(m_memory_manager, size, usage, VMA_MEMORY_USAGE_AUTO, flags);
        }
        else if (m_memory_manager)
        {
            return SparseBuffer::create(m_memory_manager, size, usage, VMA_MEMORY_USAGE_AUTO, flags);
        }
        LOG_ERROR("Memory manager is not initialized or size exceeds max allocation size, cannot create buffer");
        return nullptr;
    }

    std::shared_ptr<Buffer> Device::createWorkingBuffer(size_t size, bool is_dedicated)
    {        
        VkFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        
        if (is_dedicated) {
            flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        } else {
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        }
        
        // Add VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT flag to allow
        // fallback to transfers if memory mapping is not supported
        flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        
        return createBuffer(size, 
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                           flags);
    }

    std::shared_ptr<Buffer> Device::createSrcTransferBuffer(size_t size, bool is_dedicated)
    {
        return is_dedicated
                    ? createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT) 
                    : createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
                                VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
    }

    std::shared_ptr<Buffer> Device::createDstTransferBuffer(size_t size, bool is_dedicated)
    {
        return is_dedicated
                   ? createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                  VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                   : createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
    }

    void Device::copyData(void *src, void *dst, size_t size)
    {
        bool is_src_runtime_managed = m_buffers.find(src) != m_buffers.end();
        bool is_dst_runtime_managed = m_buffers.find(dst) != m_buffers.end();
        if (!is_src_runtime_managed && !is_dst_runtime_managed)
        {
            LOG_ERROR("DOES NOT OWN BOTH DEVICES");
        }
    }

    std::shared_ptr<Program> Device::createProgram(const std::vector<uint32_t> &shader, uint32_t dim_x, uint32_t dim_y,
                                                   uint32_t dim_z)
    {
        return Program::create(m_device, m_pipeline_cache, m_descriptorLayoutCache, m_descriptorAllocator, shader, dim_x, dim_y, dim_z);
    }

    std::shared_ptr<CommandPoolManager> runtime::Device::getComputePoolManager(size_t idx, VkQueueFlagBits flags)
    {
        auto qidx = m_queue_manager->getQueueFamilyIndex(flags);
        return CommandPoolManager::create(m_pool, m_device, idx, m_queue_manager->getQueueFamilyProperties(qidx));
    }

    void Device::submit(const std::vector<std::shared_ptr<CommandPoolManager>> &cmdPools, uint32_t i)
    {
        m_queue_manager->run(cmdPools, i);
    }    
       
    bool Device::initialize(VkInstance &instance, VkPhysicalDevice &pd, const std::vector<uint32_t> &queue_counts)
    {
        m_queue_manager = QueueManager::create(pd, queue_counts);
        // Initialize device features
        m_features = std::make_unique<DeviceFeatures>(pd);
        // Create the logical device
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = nullptr;
        m_features->getFeatures2();
        auto queueCreateInfos = m_queue_manager->getQueueCreateInfos();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        auto layerNames = m_features->getSupportedLayers();
        createInfo.ppEnabledLayerNames = layerNames.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
        
        // Get supported extensions and filter out any that aren't supported
        auto extensionNames = m_features->getSupportedExtensions();
        std::vector<const char*> enabledExtensions;
        enabledExtensions.reserve(extensionNames.size());
        
        // Only include extensions that are actually supported by the device
        uint32_t extensionCount = 0;
        std::vector<VkExtensionProperties> availableExtensions;
        
        // Get the number of extensions
        check_result(vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, nullptr), 
                    "Failed to get extension count");
        
        if (extensionCount > 0) {
            availableExtensions.resize(extensionCount);
            check_result(vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, availableExtensions.data()), 
                        "Failed to enumerate device extensions");
            
            for (const char* extName : extensionNames) {
                bool found = false;
                for (const auto& ext : availableExtensions) {
                    if (strcmp(extName, ext.extensionName) == 0) {
                        found = true;
                        enabledExtensions.push_back(extName);
                        break;
                    }
                }
                if (!found) {
                    // Log that extension wasn't found but continue
                    // We don't want to error out as some extensions might be optional
                    //LOG_INFO("Extension " + std::string(extName) + " not supported by device, ignoring");
                }
            }
        }
        
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());

        check_result(vkCreateDevice(pd, &createInfo, nullptr, &m_device), "error in creating device");
        volkLoadDevice(m_device);
        m_queue_manager->start(m_pool, pd, m_device);
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipeline_cache);

        m_memory_manager = MemoryManager::create(m_queue_manager, pd, m_device, m_features->getMaxAllocationSize());

        m_descriptorAllocator = DescriptorAllocator::create(m_device, {
                                                                          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64},
                                                                          {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8},
                                                                          {VK_DESCRIPTOR_TYPE_SAMPLER, 8},
                                                                          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
                                                                      });
        m_descriptorLayoutCache = DescriptorLayoutCache::create(m_device);

        return true;
    }

    void Device::cleanup()
    {
        if (m_buffers.size() > 0)
        {
            for (auto &buffer : m_buffers)
            {
                if (buffer.second)
                {
                    buffer.second.reset();
                }
            }
            m_buffers.clear();
        }
        m_memory_manager.reset();
        m_queue_manager.reset();

        m_descriptorLayoutCache.reset();
        m_descriptorAllocator.reset();

        if (m_pipeline_cache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(m_device, m_pipeline_cache, nullptr);
            m_pipeline_cache = VK_NULL_HANDLE;
        }

        if (m_device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
    }

}