#ifndef BARRIER_H
#define BARRIER_H

#include "error_handling.h"

#include <vulkan/vulkan.h>
#include <chrono>

namespace runtime {
// Forward declarations

// Base class for synchronization primitives
class SyncObject {
public:
    explicit SyncObject(VkDevice& device) : m_device(device) {}
    virtual ~SyncObject() = default;
    
    // Prevent copying
    SyncObject(const SyncObject&) = delete;
    SyncObject& operator=(const SyncObject&) = delete;
    
    virtual void reset() = 0;
    virtual bool isSignaled() const = 0;
    virtual void wait(uint64_t timeout = UINT64_MAX) = 0;
    virtual bool waitFor(std::chrono::nanoseconds timeout) = 0;

protected:
    VkDevice& m_device;
};

class ComputeFence : public SyncObject {
public:
    explicit ComputeFence(VkDevice& device);
    ~ComputeFence() override;

    void reset() override;
    bool isSignaled() const override;
    void wait(uint64_t timeout = UINT64_MAX) override;
    bool waitFor(std::chrono::nanoseconds timeout) override;

    VkFence getFence() const { return m_fence; }

private:
    VkFence m_fence{VK_NULL_HANDLE};
};

class DeviceSignalSemaphore : public SyncObject {   
public:
    explicit DeviceSignalSemaphore(VkDevice& device);
    ~DeviceSignalSemaphore() override;

    void reset() override;
    bool isSignaled() const override;
    void wait(uint64_t timeout = UINT64_MAX) override;
    bool waitFor(std::chrono::nanoseconds timeout) override;

    VkSemaphore getSemaphore() const { return m_semaphore; }

private:
    VkSemaphore m_semaphore{VK_NULL_HANDLE};
};

class Events : public SyncObject {
public:
    explicit Events(VkDevice& device);
    ~Events() override;

    void reset() override;
    bool isSignaled() const override;
    void wait(uint64_t timeout = UINT64_MAX) override;
    bool waitFor(std::chrono::nanoseconds timeout) override;

    VkEvent getEvent() const { return m_event; }

private:
    VkEvent m_event{VK_NULL_HANDLE};
};

// Memory barrier base class
class BarrierBase {
public:
    BarrierBase() : m_srcStageMask(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT),
                   m_dstStageMask(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT),
                   m_srcAccessMask(VK_ACCESS_MEMORY_WRITE_BIT),
                   m_dstAccessMask(VK_ACCESS_MEMORY_READ_BIT) {}
                   
    void setStageMasks(VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage) {
        m_srcStageMask = srcStage;
        m_dstStageMask = dstStage;
    }
    
    void setAccessMasks(VkAccessFlags2 srcAccess, VkAccessFlags2 dstAccess) {
        m_srcAccessMask = srcAccess;
        m_dstAccessMask = dstAccess;
    }
    
protected:
    VkPipelineStageFlags2 m_srcStageMask;
    VkPipelineStageFlags2 m_dstStageMask;
    VkAccessFlags2 m_srcAccessMask;
    VkAccessFlags2 m_dstAccessMask;
};

class MemoryBarrier : public BarrierBase {
public:
    MemoryBarrier() {
        m_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        m_memory_barrier.pNext = nullptr;
        m_memory_barrier.srcStageMask = m_srcStageMask;
        m_memory_barrier.dstStageMask = m_dstStageMask;
        m_memory_barrier.srcAccessMask = m_srcAccessMask;
        m_memory_barrier.dstAccessMask = m_dstAccessMask;
    }

    void createMemoryBarrier() {
        m_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        m_memory_barrier.srcStageMask = m_srcStageMask;
        m_memory_barrier.dstStageMask = m_dstStageMask;
    }

    void createMemoryBarrier(VkAccessFlags2 srcAccess, VkAccessFlags2 dstAccess) {
        m_memory_barrier.srcAccessMask = srcAccess;
        m_memory_barrier.dstAccessMask = dstAccess;
        m_memory_barrier.srcStageMask = m_srcStageMask;
        m_memory_barrier.dstStageMask = m_dstStageMask;
    }

    VkMemoryBarrier2* getMemoryBarrier() { return &m_memory_barrier; }

private:
    VkMemoryBarrier2 m_memory_barrier;
};

class BufferMemoryBarrier : public BarrierBase {
public:
    BufferMemoryBarrier() {
        m_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        m_buffer_memory_barrier.pNext = nullptr;
        m_buffer_memory_barrier.srcStageMask = m_srcStageMask;
        m_buffer_memory_barrier.dstStageMask = m_dstStageMask;
        m_buffer_memory_barrier.srcAccessMask = m_srcAccessMask;
        m_buffer_memory_barrier.dstAccessMask = m_dstAccessMask;
    }

    void createBufferMemoryBarrier(VkBuffer buffer, VkDeviceSize offset = 0, 
                                  VkDeviceSize size = VK_WHOLE_SIZE) {
        m_buffer_memory_barrier.srcAccessMask = m_srcAccessMask;
        m_buffer_memory_barrier.dstAccessMask = m_dstAccessMask;
        m_buffer_memory_barrier.srcStageMask = m_srcStageMask;
        m_buffer_memory_barrier.dstStageMask = m_dstStageMask;
        m_buffer_memory_barrier.buffer = buffer;
        m_buffer_memory_barrier.offset = offset;
        m_buffer_memory_barrier.size = size;
    }

    VkBufferMemoryBarrier2* getBufferMemoryBarrier() { return &m_buffer_memory_barrier; }       
private:
    VkBufferMemoryBarrier2 m_buffer_memory_barrier;
};

class ImageMemoryBarrier : public BarrierBase {
public:
    ImageMemoryBarrier() {
        m_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        m_image_memory_barrier.pNext = nullptr;
        m_image_memory_barrier.srcStageMask = m_srcStageMask;
        m_image_memory_barrier.dstStageMask = m_dstStageMask;
        m_image_memory_barrier.srcAccessMask = m_srcAccessMask;
        m_image_memory_barrier.dstAccessMask = m_dstAccessMask;
        m_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    void createImageMemoryBarrier(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                 VkImageSubresourceRange subresourceRange) {
        m_image_memory_barrier.srcAccessMask = m_srcAccessMask;
        m_image_memory_barrier.dstAccessMask = m_dstAccessMask;
        m_image_memory_barrier.srcStageMask = m_srcStageMask;
        m_image_memory_barrier.dstStageMask = m_dstStageMask;
        m_image_memory_barrier.oldLayout = oldLayout;
        m_image_memory_barrier.newLayout = newLayout;
        m_image_memory_barrier.image = image;
        m_image_memory_barrier.subresourceRange = subresourceRange;
    }

    VkImageMemoryBarrier2* getImageMemoryBarrier() { return &m_image_memory_barrier; }
private:
    VkImageMemoryBarrier2 m_image_memory_barrier;
};

} // namespace runtime

#endif // BARRIER_H