#ifndef COMPUTE_PACKET_H
#define COMPUTE_PACKET_H

#include <vulkan/vulkan.h>

namespace runtime {
    
struct ComputePacket {
    uint32_t queueFamilyIndex;
    VkCommandBuffer cmdBuffer;
    VkFence fence;
    VkSemaphore signalSemaphore;
    VkSemaphore waitSemaphore;
    VkSemaphore binaryDeviceSemaphore;  // Fixed typo
};

} // namespace runtime

#endif // COMPUTE_PACKET_H