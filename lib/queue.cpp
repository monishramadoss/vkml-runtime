// This file contains the volk implementation for the entire project
#ifndef VOLK_HH
#define VOLK_HH
#include <volk.h>
#endif // VOLK_HH
#include "queue.h"

#include "program.h"
#include "device.h"

#include "logging.h"
#include "error_handling.h"

#include <numeric>
#include <algorithm>

#include <future>

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
          m_primaryCommandBuffer(VK_NULL_HANDLE), m_threadPool(pool), m_queueFamilyIndex(queueIndex)
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

    inline uint32_t CommandPoolManager::getQueueFamilyIndex() const
    {
        return m_queueFamilyIndex;
    }

    size_t CommandPoolManager::findAvailableCommandBuffer()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // Wait with a timeout to prevent deadlock
        auto waitResult = m_cv.wait_for(lock, std::chrono::seconds(2), 
            [this] { return !used_buffers.all(); });
        
        if (!waitResult) {
            throw std::runtime_error("Timeout waiting for available command buffer");
        }

        for (size_t i = 0; i < SECONDARY_BUFFER; ++i)
        {
            if (!used_buffers[i])
            {
                used_buffers.set(i); // Mark as used
                return i;
            }
        }

        // This should never happen if the wait was successful
        throw std::runtime_error("No available command buffer found");
    }

    bool CommandPoolManager::is_ready()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return used_buffers.all() || ready; });
        if (ready)
        {
            lock.unlock();
            m_cv.notify_all();
            return true;
        }
        return false;
    }    void CommandPoolManager::set_future(const std::shared_future<int> &fut)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_fut = fut;
        m_hasFuture = true;
        m_cv.notify_all();
    }

    void CommandPoolManager::wait()
    {
        // First wait for the future to be set
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_hasFuture) {
                LOG_INFO("Waiting for future to be assigned...");
                m_cv.wait(lock, [this] { return m_hasFuture; });
            }
        }
        
        // Now that we're sure the future is valid, wait for its result
        if (m_fut.valid())
        {
            try
            {   
                std::cout << "program result " << m_fut.get() << std::endl;                
            }
            catch (const std::future_error &e)
            {
                LOG_ERROR("Future error: %s", e.what());
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("Unhandled exception: %s", e.what());
            }
        }
        else
        {
            LOG_ERROR("Future is not valid even after waiting");
        }
    }

    void CommandPoolManager::submitCompute(VkPipeline pipeline, VkPipelineLayout layout, uint32_t n_sets,
                                           const VkDescriptorSet *pDescriptors, VkPipelineBindPoint bindPoint,
                                           uint32_t dim_x, uint32_t dim_y, uint32_t dim_z)
    {
        m_threadPool->enqueue([=]() {
            size_t cmd_idx = findAvailableCommandBuffer();
            VkCommandBuffer commandBuffer = m_secondaryCommandBuffers[cmd_idx];
                
            secondaryCommandBufferRecord(commandBuffer, pipeline, layout, n_sets, pDescriptors, 
                                        bindPoint, dim_x, dim_y, dim_z);
                
            std::unique_lock<std::mutex> lock(m_mutex);
            used_buffers.reset(cmd_idx);
                
            // Check if all secondary command buffers are complete
            if (!used_buffers.any()) {
                // Create the primary command buffer only when all secondaries are done
                m_threadPool->enqueue([this]() {
                    primaryCommandBufferRecord(m_primaryCommandBuffer, 
                                            used_buffers.count(),
                                            m_secondaryCommandBuffers.data());
                    std::unique_lock<std::mutex> readyLock(m_mutex);
                    ready = true;
                    readyLock.unlock();
                    m_cv.notify_all();
                });
            }
                
            lock.unlock();
            m_cv.notify_all();
        });
    }

    void CommandPoolManager::setPromise(std::shared_ptr<std::promise<int>> promise)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_promise = promise;
    }

    bool CommandPoolManager::waitForReady(std::chrono::duration<int64_t> timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [this] { return ready; });
    }

    void CommandPoolManager::initialize(VkDevice device, uint32_t queueIndex)
    {        
        // make multiple commandPools for different tasks or asignments.. primary buffer asignments.
        VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.pNext = nullptr;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueIndex;
        check_result(vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool), "failed to construct command pool");

        VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.pNext = nullptr;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        check_result(vkAllocateCommandBuffers(device, &allocInfo, &m_primaryCommandBuffer),
                     "failed to allocate command buffer");
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        m_secondaryCommandBuffers.resize(SECONDARY_BUFFER);
        allocInfo.commandBufferCount = m_secondaryCommandBuffers.size();
        check_result(vkAllocateCommandBuffers(device, &allocInfo, m_secondaryCommandBuffers.data()),
                    "failed to allocate secondary command buffers");
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

    std::shared_ptr<QueueManager> QueueManager::create(VkPhysicalDevice &pDevice,
                                                       const std::vector<uint32_t> &queue_count)
    {
        return std::make_shared<QueueManager>(pDevice, queue_count);
    }

    QueueManager::QueueManager(VkPhysicalDevice &pDevice, const std::vector<uint32_t> &queue_count)
    {
        initialize(pDevice, queue_count);
    }

    QueueManager::~QueueManager()
    {
        cleanup();
    }
           
    VkQueue QueueManager::getSparseQueue(uint32_t i) const
    {
        return nullptr;
    }
    
    void QueueManager::start(std::shared_ptr<ThreadPool> pool, VkPhysicalDevice &pDevice, VkDevice &device)
    {
        m_threadPool = pool;
        m_device = device;
        for (int i = 0; i < m_queueCreateInfos.size(); ++i)
        {
            uint32_t queuePacketindex = m_queueFlags.front();
            m_queueFlags.pop();

            vkGetDeviceQueue(device, m_queueData[queuePacketindex] ->queueFamilyIndex,
                             m_queueData[queuePacketindex]->queueIndex, &m_queueData[queuePacketindex]->queue);
            VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fenceInfo.pNext = nullptr;
            fenceInfo.flags = 0;
            vkCreateFence(device, &fenceInfo, nullptr, &m_queueData[queuePacketindex]->fence);
            m_queueFlags.push(queuePacketindex);
        }
 
    }    void QueueManager::run(const std::vector<std::shared_ptr<CommandPoolManager>> &cmdPoolManagers, uint32_t i)
    {
        check_condition(i < m_queueData.size(), "QueueManager::run: Queue index out of range");

        // Create a shared promise that will be fulfilled when the work is complete
        auto shared_promise = std::make_shared<std::promise<int>>();
        auto shared_future = shared_promise->get_future().share();
        
        // First make sure all command pool managers have the future before starting work
        for (auto& cmd_pool : cmdPoolManagers) {
            cmd_pool->set_future(shared_future);
        }
        
        // Now submit the work to the queue
        {
            std::unique_lock<std::mutex> lock(m_workPoolM);
            m_cmdPoolQueue.push({cmdPoolManagers, shared_promise});
        }
        m_workPoolC.notify_one();

        // Second stage: Process enqueued work in a thread pool task
        m_threadPool->enqueue([this]() {
            // Get the work item
            std::vector<std::shared_ptr<CommandPoolManager>> cmd_pool_work;
            std::shared_ptr<std::promise<int>> work_promise;
            {
                std::unique_lock<std::mutex> lock(m_workPoolM);
                m_workPoolC.wait(lock, [this]() { return !m_cmdPoolQueue.empty(); });
                auto work_item = std::move(m_cmdPoolQueue.front());
                cmd_pool_work = std::move(work_item.first);
                work_promise = std::move(work_item.second);
                m_cmdPoolQueue.pop();
            }
            
            if (cmd_pool_work.empty() || !work_promise) {
                return;
            }
            
            uint32_t queuePacketindex = 0;
            // Find compatible queue
            {
                std::unique_lock<std::mutex> lock(m_QueueM);
                auto targetFamilyIndex = cmd_pool_work[0]->getQueueFamilyIndex();
                
                // Use a timeout to prevent indefinite waiting
                auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                
                while (true) {
                    // Wait for any queue to become available
                    auto waitResult = m_QueueC.wait_until(lock, deadline, [this]() { 
                        return !m_queueFlags.empty(); 
                    });
                    
                    if (!waitResult) {
                        // Timeout occurred
                        work_promise->set_exception(
                            std::make_exception_ptr(std::runtime_error("Queue wait timeout")));
                        return;
                    }
                    
                    queuePacketindex = m_queueFlags.front();
                    m_queueFlags.pop();
                    
                    // Check if this queue matches the required family index
                    if (m_queueData[queuePacketindex]->queueFamilyIndex == targetFamilyIndex) {
                        break;
                    }
                    
                    // If not compatible, put it back and continue searching
                    m_queueFlags.push(queuePacketindex);
                }
            }            // Pre-prepare each command pool
            std::vector<VkCommandBuffer> buffers;
            buffers.reserve(cmd_pool_work.size());
            
            // First pass - ensure all command pools are ready before submission
            for (auto& cmd_pool : cmd_pool_work) {
                // Share the promise with command pool - not the future, which was already set earlier
                cmd_pool->setPromise(work_promise);
                
                // Wait for the command pool to be ready (with timeout)
                if (!cmd_pool->waitForReady(std::chrono::seconds(5))) {
                    // If timeout occurs, release the queue and set exception
                    std::unique_lock<std::mutex> lock(m_QueueM);
                    m_queueFlags.push(queuePacketindex);
                    work_promise->set_exception(
                        std::make_exception_ptr(std::runtime_error("Command pool not ready in time")));
                    LOG_ERROR("Timeout waiting for command pool to be ready");
                    return;
                }
                buffers.push_back(cmd_pool->getPrimaryCommandBuffer());
            }

            // Execute the command
            try {
                submitQueue(m_queueData[queuePacketindex]->fence, m_queueData[queuePacketindex]->queue,
                            buffers.size(), buffers.data());
                vkWaitForFences(m_device, 1, &m_queueData[queuePacketindex]->fence, VK_TRUE, UINT64_MAX);
                work_promise->set_value(0); // Success
            }
            catch (const std::exception &e) {
                LOG_ERROR("Error in queue submission: %s", e.what());
                work_promise->set_exception(std::make_exception_ptr(e));
            }

            vkResetFences(m_device, 1, &m_queueData[queuePacketindex]->fence);
            {
                std::unique_lock<std::mutex> lock(m_QueueM);
                m_queueFlags.push(queuePacketindex);
            }
            m_QueueC.notify_one();
        });
    }

    uint32_t QueueManager::getQueueFamilyIndex(VkQueueFlagBits queue_flags) const
    {
        for (size_t i = 0; i < m_queueFamilies.size(); ++i)
        {
            if (m_queueFamilies[i].queueFlags & queue_flags)
            {
                return i;
            }
        }
        LOG_ERROR("Queue family index not found");
        return -1;
    }

    VkQueueFamilyProperties QueueManager::getQueueFamilyProperties(uint32_t i) const
    {
        check_condition(i < m_queueFamilies.size(), "QueueManager::getQueueFamilyProperties: Queue index out of range");
        return m_queueFamilies[i];
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
    

    bool QueueManager::initialize(VkPhysicalDevice &pDevice, const std::vector<uint32_t> &queue_count)
    { 
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies;
        queueFamilies.resize(queueFamilyCount);

        vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &queueFamilyCount, queueFamilies.data());
        size_t global_queue_count = std::accumulate(queue_count.begin(), queue_count.end(), 0);
        
        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            auto queueFamily = queueFamilies[i];
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;  
            if (queueFamily.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
                continue;
            queueCreateInfo.queueCount = std::max<uint32_t>(1, queueFamily.queueCount/4);
            float *priorites = new float[queueCreateInfo.queueCount];
            std::fill(priorites, priorites + queueCreateInfo.queueCount, 0.9f);
            queueCreateInfo.pQueuePriorities = priorites;
            m_queueCreateInfos.push_back(queueCreateInfo);
           
            m_queueFamilies.insert(m_queueFamilies.begin(), queueCreateInfo.queueCount, queueFamily);

            for (uint32_t j = 0; j < queueCreateInfo.queueCount; ++j)
            {
                m_queueFlags.push(m_queueData.size());
                m_queueData.push_back(std::make_shared<QueueData>(nullptr, nullptr, queueCreateInfo.queueFamilyIndex, j,
                                                                  std::promise<int>()));

            }
        }
        return true;
    }
    
    void QueueManager::cleanup()
    {
        while (m_queueFlags.empty() == false)
        {
            auto queueIdx = m_queueFlags.front();
            vkDestroyFence(m_device, m_queueData[queueIdx]->fence, nullptr);
            m_queueFlags.pop();
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