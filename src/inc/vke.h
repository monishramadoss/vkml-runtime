#ifndef VKE_H
#define VKE_H
#include <volk.h>
#include <vk_mem_alloc.h>
enum class vkrtType
{
    BUFFER,
    COPY_PROGRAM,
    COMPUTE_PROGRAM,
    COUNT
};

typedef struct vkrt_node
{
    vkrtType sType;
    uint64_t id = UINT64_MAX;
} vkrt_node;

typedef struct vkrt_buffer : vkrt_node
{
    VkBuffer buffer = nullptr;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo allocation_info;
} vkrt_buffer;

typedef struct vkrt_compute_program : vkrt_node
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
} vkrt_compute_program;

typedef struct vkrt_copy_packet : vkrt_node
{
    VkBuffer src;
    VkBuffer dst;
    VkBufferCopy2 copy;
} vkrt_copy_packet;

class BarrierNode
{
    VkMemoryBarrier2 m_memory_barrier;
    VkBufferMemoryBarrier2 m_buffer_memory_barrier;    
    VkImageMemoryBarrier2 m_image_memory_barrier;
    VkSemaphore m_semaphore;
    VkFence m_fence;
    VkEvent m_event;
    
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
        //VK_EVENT_CREATE_DEVICE_ONLY_BIT
        VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(device, &fenceInfo, nullptr, &m_fence);
    }
    void createEvent(VkDevice device)
    {
        //VK_EVENT_CREATE_DEVICE_ONLY_BIT
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
    
};

typedef struct ExecutionGraph
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

} ExecutionGraph;

#endif