
#include "barrier.h"


#include <vulkan/vulkan.h>
#include <chrono>
#include "error_handling.h"
#include <memory>
#include <stdexcept>

namespace runtime {

ComputeFence::ComputeFence(VkDevice& device) : SyncObject(device) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first wait succeeds
    
    VkResult result = vkCreateFence(m_device, &fenceInfo, nullptr, &m_fence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence");
    }
}

ComputeFence::~ComputeFence() {
    if (m_fence != VK_NULL_HANDLE) {
        vkDestroyFence(m_device, m_fence, nullptr);
        m_fence = VK_NULL_HANDLE;
    }
}

void ComputeFence::reset() {
    vkResetFences(m_device, 1, &m_fence);
}

bool ComputeFence::isSignaled() const {
    return vkGetFenceStatus(m_device, m_fence) == VK_SUCCESS;
}

void ComputeFence::wait(uint64_t timeout) {
    vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout);
}

bool ComputeFence::waitFor(std::chrono::nanoseconds timeout) {
    return vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, 
                          static_cast<uint64_t>(timeout.count())) == VK_SUCCESS;
}

} // namespace runtime

