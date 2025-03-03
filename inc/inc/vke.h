#ifndef VKE_H
#define VKE_H
#include <volk.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>

enum class vkrtType
{
    BUFFER,
    COPY_PROGRAM,
    COMPUTE_PROGRAM,
    COUNT
};

struct vkrt_node
{
    vkrtType sType;
    uint64_t id;

    vkrt_node(vkrtType type) : sType(type), id(UINT64_MAX) {}
};

struct vkrt_buffer : vkrt_node
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;

    vkrt_buffer() : vkrt_node(vkrtType::BUFFER), buffer(nullptr), allocation(nullptr) {}
};

struct vkrt_compute_program : vkrt_node
{
    VkShaderModule module;
    uint32_t n_sets;
    VkDescriptorSet *sets;
    VkDescriptorSetLayout *layouts;
    uint32_t *n_set_bindings;
    VkDescriptorSetLayoutBinding **bindings;   
    VkWriteDescriptorSet **writes;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    vkrt_compute_program() : vkrt_node(vkrtType::COMPUTE_PROGRAM), module(VK_NULL_HANDLE), n_sets(0), sets(nullptr), layouts(nullptr), n_set_bindings(nullptr), bindings(nullptr), writes(nullptr), pipeline(VK_NULL_HANDLE), pipeline_layout(VK_NULL_HANDLE) {}
};

struct vkrt_copy_packet : vkrt_node
{
    VkBuffer src;
    VkBuffer dst;
    VkBufferCopy2 copy;

    vkrt_copy_packet() : vkrt_node(vkrtType::COPY_PROGRAM), src(VK_NULL_HANDLE), dst(VK_NULL_HANDLE) {}
};

class BarrierNode
{
    VkMemoryBarrier2 m_memory_barrier;
    VkBufferMemoryBarrier2 m_buffer_memory_barrier;    
    VkImageMemoryBarrier2 m_image_memory_barrier;
    VkSemaphore m_semaphore;
    VkFence m_fence;
    VkEvent m_event;

public:
    BarrierNode() : m_semaphore(VK_NULL_HANDLE), m_fence(VK_NULL_HANDLE), m_event(VK_NULL_HANDLE) {}

    void createMemoryBarrier()
    {
        m_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    }

    void createBufferMemoryBarrier()
    {
        m_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    }

    void createImageMemoryBarrier()
    {
        m_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    }

    void createSemaphore(VkDevice device)
    {
        VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_semaphore);
    }

    void createFence(VkDevice device)
    {
        VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(device, &fenceInfo, nullptr, &m_fence);
    }

    void createEvent(VkDevice device)
    {
        VkEventCreateInfo eventInfo = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
        vkCreateEvent(device, &eventInfo, nullptr, &m_event);
    }

    void destroySemaphore(VkDevice device) const
    {
        vkDestroySemaphore(device, m_semaphore, nullptr);
    }

    void destroyFence(VkDevice device) const
    {
        vkDestroyFence(device, m_fence, nullptr);
    }

    void destroyEvent(VkDevice device) const
    {
        vkDestroyEvent(device, m_event, nullptr);
    }

    ~BarrierNode()
    {
        // Ensure resources are cleaned up if not already done
    }
};

struct ExecutionGraph
{
    std::vector<std::shared_ptr<vkrt_node>> m_nodes;

    void addNode(const std::shared_ptr<vkrt_node> &node)
    {
        node->id = m_nodes.size();
        m_nodes.push_back(node);
    }

    uint64_t size() const { return m_nodes.size(); }

    std::shared_ptr<vkrt_node> &getNode(uint64_t i)
    {
        return m_nodes[i];
    }
};

#endif