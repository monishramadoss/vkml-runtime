#include "queue.h"

#ifndef VOLK_HH
#define VOLK_HH
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif // VOLK_HH

#include "program.h"
#include "device.h"

#include "logging.h"
#include "error_handling.h"

#include <algorithm>
namespace runtime
{

    std::shared_ptr<CommandPoolManager> CommandPoolManager::create(std::shared_ptr<ThreadPool> pool, VkDevice device,
                                                               uint32_t queueIndex,
                                                               VkQueueFamilyProperties queueFamilyProperties)
    {
        return std::make_shared<CommandPoolManager>(pool, device, queueIndex, queueFamilyProperties);
    }

    CommandPoolManager::CommandPoolManager(std::shared_ptr<ThreadPool> pool, VkDevice device, uint32_t queueIndex,
                                           VkQueueFamilyProperties queueFamilyProperties)
        : ready(false), m_device(device), m_queueFamilyProperties(queueFamilyProperties), m_commandPool(VK_NULL_HANDLE),
          m_primaryCommandBuffer(VK_NULL_HANDLE), m_threadPool(pool)
    {
        initialize(device, queueIndex);
    }

    CommandPoolManager::~CommandPoolManager()
    {
        cleanup();
    }
       
    VkCommandPool CommandPoolManager::getCommandPool()
    {
        return m_commandPool;
    }

    VkCommandBuffer CommandPoolManager::getPrimaryCommandBuffer()
    {
        return m_primaryCommandBuffer;
    }

    std::vector<VkCommandBuffer> CommandPoolManager::getSecondaryCommandBuffer()
    {
        return m_secondaryCommandBuffers;
    }

    VkQueueFamilyProperties CommandPoolManager::getQueueFamilyProperties() const
    {
        return m_queueFamilyProperties;
    }

    size_t CommandPoolManager::findAvailableCommandBuffer()
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this] { return used_buffers.all() || ready; });

        for (size_t i = 0; i < SECONDARY_BUFFER; ++i)
        {
            if (!used_buffers[i])
            {

                used_buffers.set(i); // Mark as used
                lock.unlock();
                cv.notify_all();
                return i;
            }
        }

        // Should never reach here if used_buffers.all() check is done first
        throw std::runtime_error("No available command buffer found");
    }

    void CommandPoolManager::submitCompute(VkPipeline pipeline, VkPipelineLayout layout, uint32_t n_sets,
                                           const VkDescriptorSet *pDescriptors, VkPipelineBindPoint bindPoint,
                                           uint32_t dim_x, uint32_t dim_y, uint32_t dim_z)
    {
        m_threadPool->enqueue([=]() {
            auto cmd_idx = findAvailableCommandBuffer();
            VkCommandBuffer commandBuffer = m_secondaryCommandBuffers[findAvailableCommandBuffer()];
            secondaryCommandBufferRecord(commandBuffer, pipeline, layout, n_sets, pDescriptors, bindPoint, dim_x, dim_y,
                                         dim_z);            
            used_buffers.reset(cmd_idx);
            if (!used_buffers.any() && !ready)
            {
                std::unique_lock<std::mutex> lock(m);
                m_threadPool->enqueue([=]() {
                    primaryCommandBufferRecord(m_primaryCommandBuffer, m_secondaryCommandBuffers.size(),
                                               m_secondaryCommandBuffers.data());
                    ready = true;
                });
                lock.unlock();
            }
        });
    }

    void CommandPoolManager::initialize(VkDevice device, uint32_t queueIndex)
    {        
        // make multiple commandPools for different tasks or asignments.. primary buffer asignments.
        VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.pNext = nullptr;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueIndex;
        vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool);

        VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.pNext = nullptr;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &allocInfo, &m_primaryCommandBuffer);
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        m_secondaryCommandBuffers.resize(SECONDARY_BUFFER);
        allocInfo.commandBufferCount = m_secondaryCommandBuffers.size();
        vkAllocateCommandBuffers(device, &allocInfo, m_secondaryCommandBuffers.data());       
    }

    void CommandPoolManager::cleanup()
    {
        vkFreeCommandBuffers(m_device, m_commandPool, m_secondaryCommandBuffers.size(),
                             m_secondaryCommandBuffers.data());
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_primaryCommandBuffer);
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }

    void CommandPoolManager::secondaryCommandBufferRecord(VkCommandBuffer commandBuffer, VkPipeline pipeline,
                                                          VkPipelineLayout layout, uint32_t n_sets,
                                                          const VkDescriptorSet *pDescriptors,
                                                          VkPipelineBindPoint bindPoint, uint32_t dim_x,
                                                          uint32_t dim_y, uint32_t dim_z)
    {
        VkCommandBufferInheritanceRenderingInfo inheritanceRenderingInfo = {};
        inheritanceRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
        inheritanceRenderingInfo.colorAttachmentCount = 0;
        inheritanceRenderingInfo.pColorAttachmentFormats = nullptr;
        inheritanceRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        inheritanceRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
        inheritanceRenderingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkCommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = VK_NULL_HANDLE;
        inheritanceInfo.framebuffer = VK_NULL_HANDLE;
        inheritanceInfo.pNext = &inheritanceRenderingInfo;

        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = &inheritanceInfo;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
        vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, n_sets, pDescriptors,
                                0, nullptr);

        vkCmdDispatch(commandBuffer, dim_x, dim_y, dim_z);
        vkEndCommandBuffer(commandBuffer);
    }

    void CommandPoolManager::primaryCommandBufferRecord(VkCommandBuffer commandBuffer, uint32_t n_cmds,
                                                        const VkCommandBuffer *pCmdBuffers)
    {
        VkCommandBufferBeginInfo primaryBeginInfo = {};
        primaryBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        primaryBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        primaryBeginInfo.pNext = nullptr;
        vkBeginCommandBuffer(commandBuffer, &primaryBeginInfo);
        vkCmdExecuteCommands(commandBuffer, n_cmds, pCmdBuffers);
        vkEndCommandBuffer(commandBuffer);
    }

    std::shared_ptr<QueueManager> QueueManager::create(VkPhysicalDevice &pDevice)
    {
        return std::make_shared<QueueManager>(pDevice);
    }

    QueueManager::QueueManager(VkPhysicalDevice &pDevice)
    {
        initialize(pDevice);
    }

    QueueManager::~QueueManager()
    {
        cleanup();
    }
       
    std::shared_ptr<CommandPoolManager> QueueManager::getComputeQueue(uint32_t i)
    {
        size_t j = 0;
        std::shared_ptr<CommandPoolManager> first;
        for (auto &q : m_commandPoolManagers)
        {
            if (q->getQueueFamilyProperties().queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                if (j++ == i)
                {
                    return q;
                } 
                if (j == 0)
                    first = q;
            }
        } 
        size_t k = 0;
        for (auto &q : queueFamilies)
        {
            if (q.queueFlags & VK_QUEUE_COMPUTE_BIT && j < q.queueCount)
            {
                 m_commandPoolManagers.push_back(CommandPoolManager::create(m_threadPool, m_device, k, q));   
                 return m_commandPoolManagers.back();
            }
            ++k;
        }


        LOG_ERROR("Compute queue not found");
        return m_commandPoolManagers[i];
    }

    
    VkQueue QueueManager::getSparseQueue(uint32_t i) const
    {
        size_t j = 0;
        for (auto &q : m_commandPoolManagers)
        {
            if (q->getQueueFamilyProperties().queueFlags & VK_QUEUE_SPARSE_BINDING_BIT && q->getQueueFamilyProperties().queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                if (j++ == i)
                {
                    return m_queues[j];
                }
            }
        }
        LOG_ERROR("Sparse queue not found");
        return m_queues[i];
    }
    
    void QueueManager::start(std::shared_ptr<ThreadPool> pool, VkPhysicalDevice &pDevice, VkDevice &device)
    {
        m_threadPool = pool;
        m_device = device;
        m_fences.resize(queueFamilies.size());
        m_queues.resize(queueFamilies.size());
        // TODO need to address the problem with multiple queues in the queueFamily and adding queue
        for (size_t i = 0; i < queueFamilies.size(); ++i)
        {
            VkQueue queue;
            vkGetDeviceQueue(device, m_queueCreateInfos[i].queueFamilyIndex, 0, &m_queues[i]);
            VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fenceInfo.pNext = nullptr;
            fenceInfo.flags = 0;
            vkCreateFence(device, &fenceInfo, nullptr, &m_fences[i]);
 
            m_commandPoolManagers.push_back(
                CommandPoolManager::create(pool, device, m_queueCreateInfos[i].queueFamilyIndex, queueFamilies[i]));
        }       
    }

    void QueueManager::run(uint32_t i)
    {
        check_condition(i < m_commandPoolManagers.size(), "QueueManager::run: Queue index out of range");
        m_threadPool->enqueue([=]() {

            auto buffer = m_commandPoolManagers[i]->getPrimaryCommandBuffer();
            submitQueue(m_fences[i], m_queues[i], 1, &buffer);
        });
    }
    

    void QueueManager::submitQueue(VkFence fence, VkQueue queue, uint32_t n_cmds, const VkCommandBuffer *pCmdBuffers,
                                   const VkPipelineStageFlags *pWaitDstStageMask, uint32_t n_wait_semaphores,
                                   const VkSemaphore *wait_semaphore, uint32_t n_signal_semaphores,
                                   const VkSemaphore *signal_semaphores)
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = n_cmds;
        submitInfo.pCommandBuffers = pCmdBuffers;
        submitInfo.pWaitDstStageMask = pWaitDstStageMask;
        submitInfo.waitSemaphoreCount = n_wait_semaphores;
        submitInfo.pWaitSemaphores = wait_semaphore;
        submitInfo.signalSemaphoreCount = n_signal_semaphores;
        submitInfo.pSignalSemaphores = signal_semaphores;
        vkQueueSubmit(queue, 1, &submitInfo, fence);
    }
    

    bool QueueManager::initialize(VkPhysicalDevice &pDevice)
    { 
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
        queueFamilies.resize(queueFamilyCount);
        m_queueCreateInfos.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());
        int i = 0, j = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfo.queueCount = queueFamily.queueCount;
            float *priorites = new float[queueCreateInfo.queueCount];
            std::fill(priorites, priorites + queueCreateInfo.queueCount, 0.9f);
            queueCreateInfo.pQueuePriorities = priorites;
            m_queueCreateInfos[i++] = queueCreateInfo;
            j += queueFamily.queueCount;
        }
        return true;
    }
    
    void QueueManager::cleanup()
    {
        for (auto &q : m_commandPoolManagers)
            q.reset();
        for (auto&f : m_fences) {
            vkDestroyFence(m_device, f, nullptr);
        }
    }    
    
    std::shared_ptr<DescriptorLayoutCache> DescriptorLayoutCache::create(VkDevice device)
    {
        return std::make_shared<DescriptorLayoutCache>(device);
    }

    DescriptorLayoutCache::DescriptorLayoutCache(VkDevice device) : m_device(device)
    {
        initialize(device);
    }

    DescriptorLayoutCache::~DescriptorLayoutCache()
    {
        cleanup();
    }

    VkDescriptorSetLayout DescriptorLayoutCache::getDescriptorSetLayout(uint32_t set,
                                                                        VkDescriptorSetLayoutCreateInfo *createInfo)
    {
        if (!m_device)
        {
            throw std::runtime_error("DescriptorLayoutCache not initialized with device");
        }

        // Build layout info object for cache lookup
        DescriptorLayoutInfo layoutInfo;
        layoutInfo.setNumber = set;
        layoutInfo.bindings.reserve(createInfo->bindingCount);

        // Copy and ensure bindings are sorted
        for (uint32_t i = 0; i < createInfo->bindingCount; i++)
        {
            layoutInfo.bindings.push_back(createInfo->pBindings[i]);
        }

        // Sort bindings by binding number for consistent lookup
        std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(),
                  [](const VkDescriptorSetLayoutBinding &a, const VkDescriptorSetLayoutBinding &b) {
                      return a.binding < b.binding;
                  });

        // Check if we already have this layout cached
        auto it = m_layoutCache.find(layoutInfo);
        if (it != m_layoutCache.end())
        {
            return it->second;
        }

        // Create new layout
        VkDescriptorSetLayout layout;
        VkResult result = vkCreateDescriptorSetLayout(m_device, createInfo, nullptr, &layout);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout");
        }

        // Cache and return the new layout
        m_layoutCache[layoutInfo] = layout;
        return layout;
    }

    

    void DescriptorLayoutCache::initialize(VkDevice device)
    {

    }

    void DescriptorLayoutCache::cleanup()
    {
        for (auto &layout : m_layoutCache)
        {
            vkDestroyDescriptorSetLayout(m_device, layout.second, nullptr);
        }
        m_layoutCache.clear();
    }

    std::shared_ptr<DescriptorAllocator> DescriptorAllocator::create( VkDevice device,
        const std::vector<VkDescriptorPoolSize> &pool_sizes)
    {
        return std::make_shared<DescriptorAllocator>(device, pool_sizes);
    }

    inline DescriptorAllocator::DescriptorAllocator(VkDevice device, const std::vector<VkDescriptorPoolSize> &pool_sizes)
        : m_device(device), pool_size(pool_sizes)
    {
        if (pool_size.size() == 0)        
            throw std::runtime_error("Descriptor pool size is empty");
        
        initialize(device, pool_sizes);
    }

    DescriptorAllocator::~DescriptorAllocator()
    {
        cleanup();
    }

    inline void DescriptorAllocator::resetPools()
    {
        for (auto p : usedPools)
        {
            vkResetDescriptorPool(m_device, p, 0);
            freePools.push_back(p);
        }
        usedPools.clear();
        currentPool = VK_NULL_HANDLE;
    }

    bool DescriptorAllocator::allocate(size_t n_sets, VkDescriptorSet *set, VkDescriptorSetLayout *layout)
    {
        if (currentPool == VK_NULL_HANDLE)
            currentPool = grabPool();
        
        VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.pNext = nullptr;
        allocInfo.pSetLayouts = layout;
        allocInfo.descriptorPool = currentPool;
        allocInfo.descriptorSetCount = n_sets;

        VkResult allocResult = vkAllocateDescriptorSets(m_device, &allocInfo, set);
        bool needReallocate = false;

        switch (allocResult)
        {
        case VK_SUCCESS:
            return true;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            needReallocate = true;
            break;
        default:
            return false;
        }

        if (needReallocate)
        {
            currentPool = grabPool();
            usedPools.push_back(currentPool);
            allocInfo.descriptorPool = currentPool;
            allocResult = vkAllocateDescriptorSets(m_device, &allocInfo, set);
            if (allocResult == VK_SUCCESS)
                return true;
        }

        return false;
    }

    void DescriptorAllocator::initialize(VkDevice device, const std::vector<VkDescriptorPoolSize> &pool_sizes)
    {       
        currentPool = createPool();
        usedPools.push_back(currentPool);
    }

    inline void DescriptorAllocator::cleanup()
    {
        for (auto &pool : usedPools)
            vkDestroyDescriptorPool(m_device, pool, nullptr);
        for (auto &pool : freePools)
            vkDestroyDescriptorPool(m_device, pool, nullptr);
    }


   inline VkDescriptorPool DescriptorAllocator::createPool()
   {
       VkDescriptorPool pool;
       VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
       uint32_t count = 0;
       for (auto &p : pool_size)
           count += p.descriptorCount;
       
       pool_info.flags = 0;
       pool_info.maxSets = count;
       pool_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());
       pool_info.pPoolSizes = pool_size.data();
       vkCreateDescriptorPool(m_device, &pool_info, nullptr, &pool);
       return pool;
   }

    inline VkDescriptorPool DescriptorAllocator::grabPool()
    {
        if (freePools.size() > 0)
        {
            auto pool = freePools.back();
            freePools.pop_back();
            usedPools.push_back(pool);
            return pool;
        }
        else
        {
            auto pool = createPool();
            usedPools.push_back(pool);
            return pool;
        }
    }

   
    } // namespace runtime