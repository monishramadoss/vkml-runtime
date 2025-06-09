#include "logging.h"
#include "error_handling.h"

#include "storage.h"
#include "version.h"

#ifndef VOLK_HH
#define VOLK_HH
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif // VOLK_HH

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000
#include <vk_mem_alloc.h>

#include "queue.h"

namespace runtime
{

std::shared_ptr<MemoryManager> MemoryManager::create(std::shared_ptr<QueueManager> &queue_manager,
                                                     VkPhysicalDevice &pDevice, VkDevice &device,
                                                     size_t max_allocation_size)
{
    return std::make_shared<MemoryManager>(queue_manager, pDevice, device, max_allocation_size);
}

void MemoryManager::buildBuffer(VkBufferCreateInfo &bufferInfo, VmaAllocationCreateInfo &allocInfo, VkBuffer &buffer,
                                VmaAllocation &allocation, VmaAllocationInfo &allocationInfo)
{

    check_result(vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo),
                 "Failed to create buffer");
}

void MemoryManager::flushMemory(VmaAllocation &allocation, VkDeviceSize size, VkDeviceSize offset)
{
    check_result(vmaFlushAllocation(m_allocator, allocation, offset, size), "cache cannot be flushed correctly");
}

void MemoryManager::mapMemory(VkBuffer &buffer, VmaAllocation &allocation, void **mappedData)
{
    check_result(vmaMapMemory(m_allocator, allocation, mappedData), "Failed to map memory");
}

void MemoryManager::unmapMemory(VmaAllocation &allocation)
{
    // Unmap memory if it was previously mapped
    VmaAllocationInfo allocationInfo;
    vmaGetAllocationInfo(m_allocator, allocation, &allocationInfo);
    vmaUnmapMemory(m_allocator, allocation);
}

void MemoryManager::destroyBuffer(VkBuffer &buffer, VmaAllocation &allocation)
{
    if (buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
    }
}

void MemoryManager::buildImage(VkImageCreateInfo &imageInfo, VmaAllocationCreateInfo &allocInfo, VkImage &image,
                               VmaAllocation &allocation)
{
    check_result(vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr),
                 "Failed to create image");
}

void MemoryManager::destroyImage(VkImage &image, VmaAllocation &allocation)
{
    if (image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_allocator, image, allocation);
        image = VK_NULL_HANDLE;
    }
}

void MemoryManager::copyBuffer(VkCommandBuffer &cmd, VkBuffer &src, VkBuffer &dst, VkDeviceSize size)
{
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);
}

void MemoryManager::copyImage(VkCommandBuffer &cmd, VkImage &src, VkImage &dst, uint32_t width, uint32_t height,
                              uint32_t depth)
{
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = copyRegion.srcSubresource;
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent.width = width;
    copyRegion.extent.height = height;
    copyRegion.extent.depth = depth;
    vkCmdCopyImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                   &copyRegion);
}

void MemoryManager::copyBufferToImage(VkCommandBuffer &cmd, VkBuffer &src, VkImage &dst, uint32_t width,
                                      uint32_t height, uint32_t depth)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = depth;
    vkCmdCopyBufferToImage(cmd, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void MemoryManager::copyImageToBuffer(VkCommandBuffer &cmd, VkImage &src, VkBuffer &dst, uint32_t width,
                                      uint32_t height, uint32_t depth)
{
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = depth;
    vkCmdCopyImageToBuffer(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, 1, &region);
}


MemoryManager::MemoryManager(std::shared_ptr<QueueManager> &queue_manager, VkPhysicalDevice &pDevice, VkDevice &device,
                             size_t max_allocation_size)
    : m_queue_manager(queue_manager), m_physical_device(pDevice), m_device(device), m_allocator(nullptr)
{
    initialize(pDevice, device, max_allocation_size);
}

void MemoryManager::getVmaAllocationInfo(VmaAllocation &allocation, VmaAllocationInfo *allocationInfo) const
{
    vmaGetAllocationInfo(m_allocator, allocation, allocationInfo);
}

void MemoryManager::getVmaMemoryAllocationProperotys(VmaAllocation &allocation,
                                                     VkMemoryPropertyFlags *memoryPropertyFlags) const
{
    vmaGetAllocationMemoryProperties(m_allocator, allocation, memoryPropertyFlags);
}

inline VkQueue MemoryManager::getSparseQueue() const
{
    return m_queue_manager->getSparseQueue();
}



MemoryManager::~MemoryManager()
{
    cleanup();
}

bool MemoryManager::initialize(VkPhysicalDevice &pDevice, VkDevice &device, size_t max_allocation_size)
{
    VolkDeviceTable dt;
    volkLoadDeviceTable(&dt, device);
    VmaVulkanFunctions vmaFuncs = {};
    vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vmaFuncs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vmaFuncs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vmaFuncs.vkAllocateMemory = dt.vkAllocateMemory;
    vmaFuncs.vkFreeMemory = dt.vkFreeMemory;
    vmaFuncs.vkMapMemory = dt.vkMapMemory;
    vmaFuncs.vkUnmapMemory = dt.vkUnmapMemory;
    vmaFuncs.vkFlushMappedMemoryRanges = dt.vkFlushMappedMemoryRanges;
    vmaFuncs.vkInvalidateMappedMemoryRanges = dt.vkInvalidateMappedMemoryRanges;
    vmaFuncs.vkBindBufferMemory = dt.vkBindBufferMemory;
    vmaFuncs.vkBindImageMemory = dt.vkBindImageMemory;
    vmaFuncs.vkGetBufferMemoryRequirements = dt.vkGetBufferMemoryRequirements;
    vmaFuncs.vkGetImageMemoryRequirements = dt.vkGetImageMemoryRequirements;
    vmaFuncs.vkCreateBuffer = dt.vkCreateBuffer;
    vmaFuncs.vkDestroyBuffer = dt.vkDestroyBuffer;
    vmaFuncs.vkCreateImage = dt.vkCreateImage;
    vmaFuncs.vkDestroyImage = dt.vkDestroyImage;
    vmaFuncs.vkCmdCopyBuffer = dt.vkCmdCopyBuffer;

    auto inst = volkGetLoadedInstance();
    if (inst == VK_NULL_HANDLE)
        LOG_DEBUG("Vulkan instance is null");

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = pDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.pVulkanFunctions = &vmaFuncs;
    allocatorInfo.instance = inst;
    allocatorInfo.preferredLargeHeapBlockSize = max_allocation_size;
    allocatorInfo.vulkanApiVersion = Version::API_VERSION;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
                          VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                          VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;

    check_result(vmaCreateAllocator(&allocatorInfo, &m_allocator), "Failed to create VMA allocator");

    return true;
}

void MemoryManager::cleanup()
{
    if (m_allocator != nullptr)
    {
        vmaDestroyAllocator(m_allocator);
        m_allocator = nullptr;
    }
}

std::shared_ptr<Buffer> Buffer::create(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size,
                                       VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                                       VmaAllocationCreateFlags flags)
{
    return std::make_shared<Buffer>(mem_mamanger, size, usage, memory_usage, flags);
}

Buffer::Buffer(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
               VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags)
    : m_memory_manager(mem_mamanger), m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE)
{
    initialize(mem_mamanger, size, usage, memory_usage, flags);
}

Buffer::~Buffer()
{
    cleanup();
}

inline VkBuffer Buffer::getBuffer() const
{
    return m_buffer;
}

inline VmaAllocation Buffer::getAllocation() const
{
    return m_allocation;
}

void Buffer::copyDataFrom(void *src, size_t size, size_t dst_offset, size_t src_offset, uint32_t dst_access_flag,
                          uint32_t src_access_flag)
{
    VkBufferMemoryBarrier bufMemBarrier = {};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.buffer = m_buffer;
    bufMemBarrier.offset = dst_offset;
    bufMemBarrier.size = size;
    if (getMemoryPropertyFlags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        auto *dst = static_cast<char*>(getPtr()) + dst_offset;
        auto *sdst = std::memcpy(dst, static_cast<char *>(src) + src_offset, size);
        m_memory_manager->flushMemory(m_allocation, size, dst_offset);
        bufMemBarrier.srcAccessMask = src_access_flag;
        bufMemBarrier.dstAccessMask = dst_access_flag;
    }
    else
    {
        auto stagingBuffer = Buffer::create(m_memory_manager, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        stagingBuffer->copyDataFrom(src, size, dst_offset, src_offset, VK_ACCESS_TRANSFER_READ_BIT);
        bufMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        // submit memory copy to queue
    }
    m_buffer_memory_barriers.push_back(bufMemBarrier);
}

void Buffer::copyDataFrom(std::shared_ptr<Buffer> src, size_t size, size_t dst_offset, size_t src_offset)
{

}

void Buffer::copyDataTo(void *dst, size_t size, size_t src_offset, size_t dst_offset, uint32_t src_access_flag,
                        uint32_t dst_access_flag)
{
    VkBufferMemoryBarrier bufMemBarrier = {};
    bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufMemBarrier.buffer = m_buffer;
    bufMemBarrier.offset = src_offset;
    bufMemBarrier.size = size;
    if (getMemoryPropertyFlags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        m_memory_manager->flushMemory(m_allocation, size, src_offset);
        memcpy(static_cast<char *>(dst) + dst_offset, static_cast<char *>(getPtr()) + src_offset, size);
        bufMemBarrier.srcAccessMask = src_access_flag;
        bufMemBarrier.dstAccessMask = dst_access_flag;
    }
    else
    {
        auto stagingBuffer =
            Buffer::create(m_memory_manager, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO,
                           VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        bufMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        bufMemBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

        stagingBuffer->copyDataTo(dst, size, dst_offset, src_offset, VK_ACCESS_SHADER_WRITE_BIT,
                                  VK_ACCESS_TRANSFER_READ_BIT);
    }
}

void Buffer::copyDataTo(std::shared_ptr<Buffer> dst, size_t size, size_t dst_offset, size_t src_offset)
{
    if (getMemoryPropertyFlags() & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        // actual submission of work
        //m_memory_manager->copyBuffer(, m_buffer, dst->getBuffer(), size);
    }
}

void Buffer::initialize(std::shared_ptr<MemoryManager> &device, size_t size, VkBufferUsageFlags usage,
                        VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags)
{
    // If mapping is required, force host-preferred memory usage
    bool mapping_required = (flags & (VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT));
    if (memory_usage == VMA_MEMORY_USAGE_AUTO && mapping_required) {
        memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    }
    // Ensure host access flag is set for AUTO usage if mapping is required
    if (memory_usage == VMA_MEMORY_USAGE_AUTO || memory_usage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        if ((flags & (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)) == 0 && mapping_required) {
            // Default to sequential write for upload/staging buffers
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
    }
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.flags = flags;
    allocInfo.usage = memory_usage;
    m_memory_manager->buildBuffer(bufferInfo, allocInfo, m_buffer, m_allocation, m_allocation_info);
    m_memory_manager->getVmaMemoryAllocationProperotys(m_allocation, &m_memory_property_flags);
    if (mapping_required && !(m_memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        LOG_ERROR("Buffer allocated without HOST_VISIBLE memory but mapping was requested. Allocation will fail. Consider using a staging buffer or adjusting allocation flags.");
    }
    m_write_descriptor_set.buffer = m_buffer;
    m_write_descriptor_set.offset = 0;
    m_write_descriptor_set.range = size;
}
void Buffer::cleanup()
{
    if (m_buffer != VK_NULL_HANDLE)
    {
        m_memory_manager->destroyBuffer(m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}

std::shared_ptr<SparseBuffer> SparseBuffer::create(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size,
                                                   VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                                                   VmaAllocationCreateFlags flags)
{
    return std::make_shared<SparseBuffer>(mem_mamanger, size, usage, memory_usage, flags);
}

SparseBuffer::SparseBuffer(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
                           VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags)
    : Buffer(mem_mamanger, size, usage, memory_usage, flags)
{

}

void SparseBuffer::initialize(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
                              VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags)
{
    // If mapping is required, force host-preferred memory usage
    bool mapping_required = (flags & (VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT));
    if (memory_usage == VMA_MEMORY_USAGE_AUTO && mapping_required) {
        memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    }
    // Ensure host access flag is set for AUTO usage if mapping is required
    if (memory_usage == VMA_MEMORY_USAGE_AUTO || memory_usage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        if ((flags & (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)) == 0 && mapping_required) {
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
    }
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memory_usage;
    allocInfo.flags = flags;
    m_memory_manager->buildBuffer(bufferInfo, allocInfo, m_buffer, m_allocation, m_allocation_info);
    m_memory_manager->getVmaMemoryAllocationProperotys(m_allocation, &m_memory_property_flags);
    // RUNTIME CHECK: Ensure buffer is host-visible if mapping is required
    if ((flags & (VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)) &&
        !(m_memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        LOG_ERROR("SparseBuffer allocated without HOST_VISIBLE memory but mapping was requested. Allocation will fail. Consider using a staging buffer or adjusting allocation flags.");
    }
}

void SparseBuffer::cleanup()
{
    if (m_buffer != VK_NULL_HANDLE)
    {
        m_memory_manager->destroyBuffer(m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}



} // namespace runtime