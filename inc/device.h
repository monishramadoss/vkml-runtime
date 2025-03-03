#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <volk.h>

class BufferAllocator;
class MemoryManager;
class QueueManager;
class DeviceFeatures;

namespace runtime {
    class Device {
    public:
        Device(VkInstance& instance, VkPhysicalDevice& pd);
        ~Device();

        // Delete copy operations
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        // Enable move operations
        Device(Device&& other) noexcept;
        Device& operator=(Device&& other) noexcept;

        // Buffer operations
        std::shared_ptr<vkrt_buffer> createBuffer(const VkBufferCreateInfo* buffer_info,
                                                 VmaAllocationCreateInfo* alloc_info);
        
        // Compute program operations
        std::shared_ptr<vkrt_compute_program> createComputeProgram(const std::vector<uint32_t>& code);
        void updateDescriptorSets(uint32_t set_id, 
                                 std::shared_ptr<vkrt_compute_program> program,
                                 const std::vector<std::shared_ptr<vkrt_buffer>>& buffers);

        // Memory operations
        void mapBuffer(VmaAllocation alloc, void** data) const;
        void unmapBuffer(VmaAllocation alloc) const;
        VkResult copyMemoryToBuffer(VmaAllocation dst, const void* src, 
                                   size_t size, size_t dst_offset = 0) const;
        VkResult copyBufferToMemory(VmaAllocation src, void* dst, 
                                   size_t size, size_t src_offset = 0) const;
        
        // Event operations
        void createEvent(const VkEventCreateInfo* info, VkEvent* event) const;
        void destroyEvent(VkEvent event) const;

        // Resource cleanup
        void destroy();

        // Getters
        const DeviceFeatures& getFeatures() const { return *m_features; }
        VkDevice getDevice() const { return m_device; }
        VmaAllocator getAllocator() const { return m_buffer_allocator->getMemoryManager()->; }

        void submitBlobs(std::vector<uint32_t>& blobs);


    private:
        void initialize(VkInstance& instance, VkPhysicalDevice& pd);
        void cleanup();

        std::shared_ptr<BufferAllocator> m_buffer_allocator {nullptr};
        std::shared_ptr<MemoryManager> m_memory_manager {nullptr};
        std::shared_ptr<QueueManager> m_queue_manager {nullptr};
        std::unique_ptr<DeviceFeatures> m_features;
        
        VkDevice m_device{VK_NULL_HANDLE};
        VkPipelineCache m_pipeline_cache{VK_NULL_HANDLE};
        VkPhysicalDevice m_physical_device{VK_NULL_HANDLE};
        VmaAllocator m_allocator{VK_NULL_HANDLE};

    };


    
} // namespace runtime
