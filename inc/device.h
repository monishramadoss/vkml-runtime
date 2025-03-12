#ifndef DEVICE_H
#define DEVICE_H

#include "config.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <iostream>


namespace runtime {

// Forward declarations
class BufferAllocator;
class MemoryManager;
class QueueManager;
class DeviceFeatures;
class DescriptorManager;
class ComputeProgram;
class Buffer;
class Image;
class ImageAllocator;

class Device {
public:
    static std::shared_ptr<Device> create(VkInstance& instance, VkPhysicalDevice& pd);

    // Delete copy operations
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Enable move operations
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

   
    // Buffer operations
    auto createBuffer(uint32_t bufferinfo, bool isHostVisible);
    
    // Image operations
    std::shared_ptr<Image> createImage(const VkImageCreateInfo& imageInfo);
    
    // Compute program operations
    std::shared_ptr<ComputeProgram> createComputeProgram();
    void updateDescriptorSets(uint32_t set_id, 
                             std::shared_ptr<ComputeProgram> program,
                             const std::vector<std::shared_ptr<Buffer>>& buffers);

    // Memory operations
    void mapBuffer(VmaAllocation alloc, void** data) const;
    void unmapBuffer(VmaAllocation alloc) const;
    VkResult copyMemoryToBuffer(VmaAllocation dst, const void* src, 
                               size_t size, size_t dst_offset = 0) const;
    VkResult copyBufferToMemory(VmaAllocation src, void* dst, 
                               size_t size, size_t src_offset = 0) const;
    
    // Command submission helper
    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer cmdBuffer) const;
    
    // Event operations
    void createEvent(const VkEventCreateInfo* info, VkEvent* event) const;
    void destroyEvent(VkEvent event) const;

    // Resource cleanup
    void destroy();

    // Getters
    const DeviceFeatures& getDeviceFeatures() const { return *m_features; }
    VkDevice getDevice() const { return m_device; }
    VmaAllocator getAllocator() const;
    VkQueue getComputQueue() const;
    uint32_t getComputeQueueFamily() const;
    
    VkQueue getTransferQueue() const;
    uint32_t getTransferQueueFamily() const;
    
    DescriptorManager& getDescriptorManager() const {
        if (!m_descriptor_manager) {
            std::cout << "Descriptor manager not initialized" << std::endl;
        }
        return *m_descriptor_manager;
    }

    void submitBlobs(std::vector<uint32_t>& blobs);

private:
    Device(VkInstance& instance, VkPhysicalDevice& pd);
    ~Device();
    // Initialize device
    bool initialize(const std::string& appName = "VKML Runtime", bool enableValidation = false);
    void cleanup();


    // Device selection
    int rateDevice(VkPhysicalDevice device);
    bool selectPhysicalDevice();
    bool findQueueFamilies();
    bool createLogicalDevice();

    void initialize(VkInstance& instance, VkPhysicalDevice& pd);
    void createAllocators();

    std::shared_ptr<BufferAllocator> m_buffer_allocator{nullptr};
    std::shared_ptr<ImageAllocator> m_image_allocator{nullptr};
    std::shared_ptr<MemoryManager> m_memory_manager{nullptr};
    std::shared_ptr<QueueManager> m_queue_manager{nullptr};
    std::unique_ptr<DeviceFeatures> m_features;
    std::unique_ptr<DescriptorManager> m_descriptor_manager;

    VkDevice m_device{VK_NULL_HANDLE};
    VmaAllocator m_allocator{VK_NULL_HANDLE};

    bool m_validationEnabled{false};
};

} // namespace runtime


#endif // DEVICE_H