#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>

#include <array>
#include <bitset>
#include <condition_variable>
#include <future>
#include <queue>
#include <stdio.h>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <volk.h>
constexpr int MAX_CMD_POOL_SZ = 4;
constexpr int MAX_QUE_COUNT = 31;
// TODO make submission thread, and command buffer generation thread;
// TODO make a descriptor pool per thread;

namespace runtime
{
unsigned int getFirstSetBitPos(int n)
{
    return static_cast<unsigned int>(log2(n & -n)) + 1;
}

struct ComputePacket
{
    VkShaderStageFlagBits flags;
    uint32_t dims[3];
    VkPipeline pipeline;
    VkPipelineLayout layout;
    uint32_t n_sets;
    VkDescriptorSet* sets;
};

template <typename T, size_t BufferSize> class threadsafe_queue
{
    std::mutex m_mutex;
    std::condition_variable m_cond_full;
    std::condition_variable m_cond_empty;
    std::array<T, BufferSize + 1> m_queue;
    std::atomic<size_t> m_start{0};
    std::atomic<size_t> m_end{0};

    size_t increment(size_t idx) const
    {
        return (idx + 1) % (BufferSize + 1);
    }

  public:
    bool push(const T &packet)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_full.wait(lock, [this]() { return !isFull(); });
        auto current_end = m_end.load();
        m_queue[current_end] = packet;
        m_end.store(increment(current_end));
        m_cond_empty.notify_all();
        return true;
    }

    bool pop(T &packet, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cond_empty.wait_for(lock, timeout, [this]() { return !isEmpty(); }))
        {
            return false; // Timeout occurred
        }

        packet = m_queue[m_start];
        m_start.store(increment(m_start.load()));
        m_cond_full.notify_one();
        return true;
    }

    bool isEmpty() const
    {
        return m_start.load() == m_end.load();
    }

    bool isFull() const
    {
        auto next_end = increment(m_end.load());
        return next_end == m_start.load();
    }
};

template <size_t N> struct cmd_t
{
    VkQueue m_queue;
    VkFence m_fence;
    VkCommandPool m_pool;
    VkCommandBuffer m_primary_buffer;
    VkCommandBuffer m_secondary_buffer[N];
    std::condition_variable m_cv;
    threadsafe_queue<int, N> m_available_pool;
    threadsafe_queue<int, N> m_ready_pool;
    threadsafe_queue<int, N> m_working_pool;
    void create(VkDevice device, uint32_t i)
    {
        vkGetDeviceQueue(device, i, 0, &m_queue);

        VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.pNext = nullptr;
        fenceInfo.flags = 0;
        vkCreateFence(device, &fenceInfo, nullptr, &m_fence);

        VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.pNext = nullptr;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = i;
        vkCreateCommandPool(device, &poolInfo, nullptr, &m_pool);

        VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.pNext = nullptr;
        allocInfo.commandPool = m_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device, &allocInfo, &m_primary_buffer);
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        allocInfo.commandBufferCount = N;
        vkAllocateCommandBuffers(device, &allocInfo, m_secondary_buffer);
        for (auto i = 0; i < N; ++i)
        {
            m_available_pool.push(i);
        }
    }

    void destroy(VkDevice device) const
    {
        vkFreeCommandBuffers(device, m_pool, N, m_secondary_buffer);
        vkFreeCommandBuffers(device, m_pool, 1, &m_primary_buffer);
        vkDestroyCommandPool(device, m_pool, nullptr);
        vkDestroyFence(device, m_fence, nullptr);
    }
};

static void secondaryBufferRecord(ComputePacket &packet, VkCommandBuffer &cmdBuffer)
{
    VkCommandBufferInheritanceRenderingInfo inheritanceRenderingInfo = {};
    inheritanceRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
    inheritanceRenderingInfo.colorAttachmentCount = 0;
    inheritanceRenderingInfo.pColorAttachmentFormats = nullptr;
    inheritanceRenderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    inheritanceRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    inheritanceRenderingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkCommandBufferInheritanceInfo inheritanceInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};
    inheritanceInfo.renderPass = VK_NULL_HANDLE;
    inheritanceInfo.framebuffer = VK_NULL_HANDLE;
    inheritanceInfo.pNext = &inheritanceRenderingInfo;
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, packet.pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, packet.layout, 0, packet.n_sets,
                            packet.sets, 0, nullptr);

    vkCmdDispatch(cmdBuffer, packet.dims[0], packet.dims[1], packet.dims[2]);
    vkEndCommandBuffer(cmdBuffer);
   
}

static void primaryBufferRecord(VkCommandBuffer &primaryBuffer, const std::vector<VkCommandBuffer> &cmdBuffers)
{
    VkCommandBufferBeginInfo primaryBeginInfo = {};
    primaryBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    primaryBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    primaryBeginInfo.pNext = nullptr;
    vkBeginCommandBuffer(primaryBuffer, &primaryBeginInfo);
    vkCmdExecuteCommands(primaryBuffer, static_cast<unsigned int>(cmdBuffers.size()), cmdBuffers.data());
    vkEndCommandBuffer(primaryBuffer);
}

static void submitQueue(VkQueue &queue, const std::vector<VkCommandBuffer> &primaryBuffer, VkFence &fence)
{
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.commandBufferCount = static_cast<unsigned int>(primaryBuffer.size());
    submitInfo.pCommandBuffers = primaryBuffer.data();
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    vkQueueWaitIdle(queue);
    vkQueueSubmit(queue, 1, &submitInfo, fence);
}

template <size_t N>
static void recorderThread(threadsafe_queue<ComputePacket, 32> &compute_packets, cmd_t<N> &worker_data, bool stop)
{
    const std::chrono::milliseconds timeout(100);
    while (true)
    {
        if (stop)
            return;
        ComputePacket packet;
        int i = -1;
        if (compute_packets.pop(packet, timeout) && worker_data.m_available_pool.pop(i, timeout))
        {
            auto &buffer = worker_data.m_secondary_buffer[i];
            secondaryBufferRecord(packet, buffer);
            worker_data.m_ready_pool.push(i);
        }
    }
}

template <size_t N>
static void submissionThread(cmd_t<N> &worker_data, std::mutex &mut, std::condition_variable &cv, bool ready_to_wait,
                             bool stop)
{
    const std::chrono::milliseconds timeout(100);
    std::vector<VkCommandBuffer> cmd_buffers;
    std::queue<size_t> cmd_buffer_indices;
    bool start = false;
    while (true)
    {
        if (stop)
            return;
        int j = -1;
        if (worker_data.m_ready_pool.pop(j, timeout))
        {
            cmd_buffers.push_back(worker_data.m_secondary_buffer[j]);
            cmd_buffer_indices.push(j);
            start = true;
        }

        if (start)
        {
            primaryBufferRecord(worker_data.m_primary_buffer, cmd_buffers);
            submitQueue(worker_data.m_queue, {worker_data.m_primary_buffer}, worker_data.m_fence);
            cmd_buffers.clear();
            while (!cmd_buffer_indices.empty())
            {
                worker_data.m_working_pool.push(cmd_buffer_indices.front());
                cmd_buffer_indices.pop();
            }
            start = false;
        }
    }
}

template <size_t N>
static void syncThread(VkDevice &device, cmd_t<N> &worker_data, std::mutex &mut, std::condition_variable &cv,
                       bool ready_to_wait, bool stop)
{
    const std::chrono::milliseconds timeout(100);
    int j = -1;
    while (true)
    {

        while (!worker_data.m_working_pool.isEmpty())
        {
            if (worker_data.m_working_pool.pop(j, timeout))
            {
                vkResetCommandBuffer(worker_data.m_secondary_buffer[j], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
                worker_data.m_available_pool.push(j);
            }
        }
    }
}


template <size_t N> class WorkerQueuePool
{
    std::thread submission_thread;
    std::thread sync_thread;
    std::vector<std::thread> recorders;
    threadsafe_queue<ComputePacket, 32> compute_packets;
    cmd_t<N> worker_data;
    std::vector<VkCommandBuffer> cmd_buffers;
    bool stop = false;
    bool ready_to_wait = false;
    std::mutex working_mutex;
    std::condition_variable working_cv;

  public:
    void create(VkDevice device, uint32_t i)
    {
        worker_data.create(device, i);
        recorders.resize(N);
        for (auto j = 0; j < 1; ++j)
        {
            recorders[j] =
                std::thread(recorderThread<N>, std::ref(compute_packets), std::ref(worker_data), std::ref(stop));
        }
        submission_thread = std::thread(submissionThread<N>, std::ref(worker_data), std::ref(working_mutex),
                                        std::ref(working_cv), std::ref(ready_to_wait), std::ref(stop));        
    }

    void submit(ComputePacket &packet)
    {
        compute_packets.push(packet);
      
    }

    void destroy(VkDevice device)
    {
        stop = true;
        if (submission_thread.joinable())
            submission_thread.join();
        if (sync_thread.joinable())
            sync_thread.join();
        for (auto &recorder : recorders)
        {
            if (recorder.joinable())
                recorder.join();
        }

        worker_data.destroy(device);
    }
};

class QueueDispatcher
{
    VkQueueFamilyProperties m_properties;
    uint32_t m_idx;
    WorkerQueuePool<MAX_CMD_POOL_SZ> m_workers[MAX_QUE_COUNT];
    size_t m_max_units;
    VkDevice m_device;

    std::vector<std::thread> m_recorderThreads;
    std::vector<std::thread> m_submissionThreads;

  public:
    bool isGraphicFamily() const
    {
        return (m_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT;
    }
    bool isComputeFamily() const
    {
        return (m_properties.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT;
    }
    bool isTransferFamily() const
    {
        return (m_properties.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT;
    }
    bool isSparseBindingFamily() const
    {
        return (m_properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_SPARSE_BINDING_BIT;
    }
    bool isVideoDecodeFamily() const
    {
        return (m_properties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) == VK_QUEUE_VIDEO_DECODE_BIT_KHR;
    }
    bool isVideoEncodeFamily() const
    {
        return (m_properties.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) == VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    }
    bool isOtherFamily() const
    {
        return ~(VK_QUEUE_TRANSFER_BIT & VK_QUEUE_COMPUTE_BIT & VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_SPARSE_BINDING_BIT &
                 VK_QUEUE_VIDEO_DECODE_BIT_KHR & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) &
               m_properties.queueFlags;
    }

    QueueDispatcher(uint32_t idx, VkDevice device, VkQueueFamilyProperties &properties)
        : m_properties(properties), m_idx(idx), m_device(device)
    {
        m_max_units = 1;
        m_workers[0].create(device, idx);
    }

    auto get_idx() const
    {
        return m_idx;
    }

    void destroy(VkDevice device)
    {
        for (auto i = 0; i < m_max_units; ++i)
            m_workers[i].destroy(device);
    }

    void submit(ComputePacket &packet)
    {
        m_workers[0].submit(packet);       
    }

    bool isBusy() const
    {
        return false;
    }

};


} // namespace vkrt