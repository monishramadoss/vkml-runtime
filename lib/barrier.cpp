#include "barrier.h"
#include "device.h"
#include "buffer.h"


#include <vulkan/vulkan.h>
#include <chrono>
#include "../error_handling.h"
#include <memory>

namespace runtime {

ComputeFence::ComputeFence(Device& device)
    : m_device(device)
{
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(m_device.getHandle(), &fenceInfo, nullptr, &m_fence));
}

ComputeFence::~ComputeFence()
{
    vkDestroyFence(m_device.getHandle(), m_fence, nullptr);
}

void ComputeFence::reset()
{
    VK_CHECK(vkResetFences(m_device.getHandle(), 1, &m_fence));
}

bool ComputeFence::isSignaled() const
{
    VkResult result = vkGetFenceStatus(m_device.getHandle(), m_fence);
    return result == VK_SUCCESS;
}

void ComputeFence::wait(uint64_t timeout)
{
    VK_CHECK(vkWaitForFences(m_device.getHandle(), 1, &m_fence, VK_TRUE, timeout));
}

bool ComputeFence::waitFor(std::chrono::nanoseconds timeout)
{
    return waitFor(static_cast<uint64_t>(timeout.count()));
}

DeviceSignalSemaphore::DeviceSignalSemaphore(Device& device)
    : m_device(device)
{
    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(vkCreateSemaphore(m_device.getHandle(), &semaphoreInfo, nullptr, &m_semaphore));   
}

DeviceSignalSemaphore::~DeviceSignalSemaphore()
{
    vkDestroySemaphore(m_device.getHandle(), m_semaphore, nullptr);
}

void DeviceSignalSemaphore::reset()
{
    // No-op
}   

bool DeviceSignalSemaphore::isSignaled() const
{
    // No-op
    return true;
}

void DeviceSignalSemaphore::wait(uint64_t timeout){
    // No-op
}

bool DeviceSignalSemaphore::waitFor(std::chrono::nanoseconds timeout){
    // No-op
    return true;
}

Events::Events(Device& device)
    : m_device(device)
{
    VkEventCreateInfo eventInfo = { VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
    VK_CHECK(vkCreateEvent(m_device.getHandle(), &eventInfo, nullptr, &m_event));
}

Events::~Events()
{
    vkDestroyEvent(m_device.getHandle(), m_event, nullptr);

}

void Events::reset()
{
    VK_CHECK(vkResetEvent(m_device.getHandle(), m_event));

}

bool Events::isSignaled() const
{
    VkResult result = vkGetEventStatus(m_device.getHandle(), m_event);
    return result == VK_EVENT_SET;

}

void Events::wait(uint64_t timeout)
{
    VK_CHECK(vkWaitForFences(m_device.getHandle(), 1, &m_event, VK_TRUE, timeout));
}

bool Events::waitFor(std::chrono::nanoseconds timeout)
{
    return waitFor(static_cast<uint64_t>(timeout.count()));

}

} // namespace runtime

