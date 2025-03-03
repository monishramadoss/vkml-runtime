#pragma once
#include <vulkan/vulkan.h>
#include <chrono>
#include "../error_handling.h"
#include <memory>

namespace runtime {

class Device;

class ComputeFence
{
public:
    explicit ComputeFence(Device& device);
    ~ComputeFence();

    // Prevent copying
    ComputeFence(const ComputeFence&) = delete;
    ComputeFence& operator=(const ComputeFence&) = delete;

    void reset();
    bool isSignaled() const;
    
    void wait(uint64_t timeout = UINT64_MAX);
    bool waitFor(std::chrono::nanoseconds timeout);

    VkFence getFence() const { return m_fence; }

private:
    Device& m_device;
    VkFence m_fence{VK_NULL_HANDLE};
};

class DeviceSignalSemaphore
{   
public:
    explicit DeviceSignalSemaphore(Device& device);
    ~DeviceSignalSemaphore();

    // Prevent copying
    DeviceSignalSemaphore(const DeviceSignalSemaphore&) = delete;
    DeviceSignalSemaphore& operator=(const DeviceSignalSemaphore&) = delete;

    void reset();
    bool isSignaled() const;
    
    void wait(uint64_t timeout = UINT64_MAX);
    bool waitFor(std::chrono::nanoseconds timeout);

    VkSemaphore getSemaphore() const { return m_semaphore; }

private:
    Device& m_device;
    VkSemaphore m_semaphore{VK_NULL_HANDLE};
};

class Events
{
public:
    explicit Events(Device& device);
    ~Events();

    // Prevent copying
    Events(const Events&) = delete;
    Events& operator=(const Events&) = delete;

    void reset();
    bool isSignaled() const;
    
    void wait(uint64_t timeout = UINT64_MAX);
    bool waitFor(std::chrono::nanoseconds timeout);

    VkEvent getEvent() const { return m_event; }

private:
    Device& m_device;
    VkEvent m_event{VK_NULL_HANDLE};
};

class MemoryBarrier {
public:
    MemoryBarrier() {
        m_memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        m_memory_barrier.pNext = nullptr;
        m_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        m_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        m_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    void createMemoryBarrier() {
        m_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    void createMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess) {
        m_memory_barrier.srcAccessMask = srcAccess;
        m_memory_barrier.dstAccessMask = dstAccess;
    }

    VkMemoryBarrier2* getMemoryBarrier() { return &m_memory_barrier; }

private:
    
    VkMemoryBarrier2 m_memory_barrier;
};

class BufferMemoryBarrier {
public:
    BufferMemoryBarrier() {
        m_buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        m_buffer_memory_barrier.pNext = nullptr;
        m_buffer_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        m_buffer_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        m_buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_buffer_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    void createBufferMemoryBarrier() {
        m_buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_buffer_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    void createBufferMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess) {
        m_buffer_memory_barrier.srcAccessMask = srcAccess;
        m_buffer_memory_barrier.dstAccessMask = dstAccess;
    }

    VkBufferMemoryBarrier2* getBufferMemoryBarrier() { return &m_buffer_memory_barrier; }       
private:
    
    VkBufferMemoryBarrier2 m_buffer_memory_barrier;
};

class ImageMemoryBarrier {
public:
    ImageMemoryBarrier() {
        m_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        m_image_memory_barrier.pNext = nullptr;
        m_image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        m_image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        m_image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    void createImageMemoryBarrier() {
        m_image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        m_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    }

    void createImageMemoryBarrier(VkAccessFlags srcAccess, VkAccessFlags dstAccess) {
        m_image_memory_barrier.srcAccessMask = srcAccess;
        m_image_memory_barrier.dstAccessMask = dstAccess;
    }

    VkImageMemoryBarrier2* getImageMemoryBarrier() { return &m_image_memory_barrier; }
private:
    
    VkImageMemoryBarrier2 m_image_memory_barrier;
};

} // namespace runtime