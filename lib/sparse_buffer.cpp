#include "sparse_buffer.h"
#include "buffer_allocator.h"
#include <stdexcept>
#include <iostream>
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace runtime {

std::shared_ptr<SparseBuffer> SparseBuffer::create(
    std::shared_ptr<BufferAllocator> allocator,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const std::vector<uint32_t>& sparseQueueFamilies)
{
    return std::shared_ptr<SparseBuffer>(new SparseBuffer(allocator, size, usage, sparseQueueFamilies));
}

SparseBuffer::SparseBuffer(std::shared_ptr<BufferAllocator> allocator,
                           VkDeviceSize size,
                           VkBufferUsageFlags usage,
                           const std::vector<uint32_t>& sparseQueueFamilies)
    : Buffer(allocator, size, usage, false),  // Base constructor
      m_sparseQueueFamilies(sparseQueueFamilies)
{
    
}

SparseBuffer::~SparseBuffer()
{
    // No need to explicitly unbind sparse memory - will be handled by parent class cleanup
}

void SparseBuffer::initialize()
{
  
}

void SparseBuffer::bindMemory(const std::vector<MemoryBinding>& bindings, VkQueue sparseQueue, VkFence fence)
{
    if (m_sparseBinding) {
        throw std::runtime_error("Memory already bound to sparse buffer");
    }

    if (bindings.empty()) {
        throw std::invalid_argument("No memory bindings provided");
    }

    // Create sparse memory binding
    std::vector<VkSparseMemoryBind> memoryBinds;
    for (const auto& binding : bindings) {
        VkSparseMemoryBind memoryBind {};
        memoryBind.memory = binding.memory;
        memoryBind.memoryOffset = binding.memoryOffset;
        memoryBind.resourceOffset = binding.resourceOffset;
        memoryBind.size = binding.size;
        memoryBind.flags = binding.flags;
        memoryBinds.push_back(memoryBind);
    }

    VkSparseBufferMemoryBindInfo bufferBindInfo {};
    bufferBindInfo.buffer = m_buffer;
    bufferBindInfo.bindCount = static_cast<uint32_t>(memoryBinds.size());
    bufferBindInfo.pBinds = memoryBinds.data();

    VkBindSparseInfo bindSparseInfo {};
    bindSparseInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
    bindSparseInfo.bufferBindCount = 1;
    bindSparseInfo.pBufferBinds = &bufferBindInfo;

    VkFence fences[] = {fence};
    bindSparseInfo.signalSemaphoreCount = 0;
    bindSparseInfo.waitSemaphoreCount = 0;
    bindSparseInfo.pSignalSemaphores = nullptr;
    bindSparseInfo.pWaitSemaphores = nullptr;
    bindSparseInfo.pFence = fences;

    VkResult result = vkQueueBindSparse(sparseQueue, 1, &bindSparseInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind sparse memory: " + std::to_string(result));
    }

    m_sparseBinding = true;
    m_memoryBindings = memoryBinds;
}
} // namespace runtime

