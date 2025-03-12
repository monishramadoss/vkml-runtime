#ifndef BUFFER_ALLOCATOR_H
#define BUFFER_ALLOCATOR_H

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <unordered_map>
#include <memory>

namespace runtime {

class MemoryManager;

class BufferAllocator {
public:
    friend static std::shared_ptr<BufferAllocator> create(VkDevice device, const std::shared_ptr<MemoryManager>& allocator);
    

    // Create/destroy buffers
    VkResult createBuffer(VkDeviceSize size, 
                         VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         VkBuffer* buffer);
    void destroyBuffer(VkBuffer* buffer);
    
    // Memory mapping
    void* mapMemory(VkBuffer* buffer);
    void unmapMemory(VkBuffer* buffer);
    void flushCacheToMemory(VkBuffer* buffer, size_t offset, size_t size);
    void invalidateCache(VkBuffer* buffer, size_t offset, size_t size);
    
    explicit BufferAllocator(VkDevice, const std::shared_ptr<MemoryManager>&);    
    ~BufferAllocator();
    
    // Get the device
    VkDevice getDevice() const;
private:

    void cleanup();
    
    VkDevice m_device;
    std::shared_ptr<MemoryManager> m_memory_manager{VK_NULL_HANDLE};
    std::unordered_map<VkBuffer, VmaAllocation> m_allocations;
    bool m_initialized{false};
};

} // namespace runtime

#endif // BUFFER_ALLOCATOR_H