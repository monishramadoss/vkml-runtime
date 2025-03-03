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
    : Buffer(allocator, size, usage, false),
      m_sparseQueueFamilies(sparseQueueFamilies)
{
    initializeSparse();
}

SparseBuffer::~SparseBuffer()
{
    if (m_sparseBinding) {
        vkQueueBindSparse(m_allocator->getMemoryManager()->getQueue(), 0, nullptr, VK_NULL_HANDLE, VK_NULL_HANDLE);
    }
}

void SparseBuffer::initializeSparse()
{
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = m_size;
    bufferInfo.usage = m_usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    m_allocator->allocateBuffer(bufferInfo, allocInfo, m_buffer, m_allocation, m_allocationInfo);

    m_sparseBinding = true;
}

void SparseBuffer::bindMemory(const std::vector<MemoryBinding>& bindings)
{
    if (!m_sparseBinding) {
        throw std::runtime_error("Buffer is not sparse bound");
    }

    std::vector<VkSparseMemoryBind> memoryBinds;
    for (const auto& binding : bindings) {
        VkSparseMemoryBind memoryBind = {};
        memoryBind.resourceOffset = binding.resourceOffset;
        memoryBind.size = binding.size;
        memoryBind.memory = binding.memory;
        memoryBind.memoryOffset = binding.memoryOffset;
        memoryBinds.push_back(memoryBind);
    }

    VkSparseBufferMemoryBindInfo bufferBindInfo = {};
    bufferBindInfo.buffer = m_buffer;
    bufferBindInfo.bindCount = static_cast<uint32_t>(memoryBinds.size());
    bufferBindInfo.pBinds = memoryBinds.data();

    VkBindSparseInfo bindSparseInfo = {VK_STRUCTURE_TYPE_BIND_SPARSE_INFO};
    bindSparseInfo.bufferBindCount = 1;
    bindSparseInfo.pBufferBinds = &bufferBindInfo;

    VkFence fence = VK_NULL_HANDLE;
    VkResult result = vkQueueBindSparse(m_allocator->getMemoryManager()->getQueue(), 1, &bindSparseInfo, fence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind sparse memory");
    }
}

} // namespace runtime

