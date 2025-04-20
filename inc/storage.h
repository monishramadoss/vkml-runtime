#ifndef MEMORY_H
#define MEMORY_H

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

#ifndef VMA_HH
#define VMA_HH

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_VULKAN_VERSION 1003000


#define VMA_USE_STL_CONTAINERS 0
#define VMA_USE_STL_VECTOR 0
#define VMA_USE_STL_UNORDERED_MAP 0
#define VMA_USE_STL_LIST 0
#define VMA_USE_STL_DEQUE 0

#include <vk_mem_alloc.h>

#endif // VMA_HH

#include <memory>
#include <vector>
#include <unordered_map>

namespace runtime
{
class Buffer;
class SparseBuffer;
class BufferView;
class Image;
class SparseImage;
class ImageView;

class QueueManager;
    
    typedef struct buffer_memory
    {
        VkBuffer m_buffer{VK_NULL_HANDLE};
        VmaAllocation m_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo m_allocation_info{};
    } buffer_memory_t;

    

	class MemoryManager
	{
      public:
        static std::shared_ptr<MemoryManager> create(std::shared_ptr<QueueManager> &queue_manager,
                                                     VkPhysicalDevice &pDevice, VkDevice &device,
                                                     size_t max_allocation_size);

        void buildBuffer(VkBufferCreateInfo &bufferInfo, VmaAllocationCreateInfo &allocInfo, VkBuffer &buffer,
                         VmaAllocation &allocation, VmaAllocationInfo &allocationInfo);
        void flushMemory(VmaAllocation &allocation, VkDeviceSize size, VkDeviceSize offset);
        void mapMemory(VkBuffer& buffer, VmaAllocation& allocation, void** mappedData);
        void unmapMemory(VmaAllocation &buffer);
        void destroyBuffer(VkBuffer &buffer, VmaAllocation &allocation);

        void buildImage(VkImageCreateInfo &imageInfo, VmaAllocationCreateInfo &allocInfo, VkImage &image,
                        VmaAllocation &allocation);
        void destroyImage(VkImage &image, VmaAllocation &allocation);

        void copyBuffer(VkCommandBuffer &cmd, VkBuffer &src, VkBuffer &dst, VkDeviceSize size);
        void copyImage(VkCommandBuffer &cmd, VkImage &src, VkImage &dst, uint32_t width, uint32_t height,
                       uint32_t depth);

        void copyBufferToImage(VkCommandBuffer &cmd, VkBuffer &src, VkImage &dst, uint32_t width, uint32_t height,
                               uint32_t depth);

        void copyImageToBuffer(VkCommandBuffer &cmd, VkImage &src, VkBuffer &dst, uint32_t width, uint32_t height,
            uint32_t depth);

        MemoryManager(std::shared_ptr<QueueManager> &queue_manager, VkPhysicalDevice &pDevice, VkDevice &device,
                      size_t max_allocation_size);

        void getVmaAllocationInfo(VmaAllocation &allocation, VmaAllocationInfo *allocationInfo) const;
        void getVmaMemoryAllocationProperotys(VmaAllocation &allocation,
                                              VkMemoryPropertyFlags *memoryPropertyFlags) const;
        VkQueue getSparseQueue() const;

        ~MemoryManager();
	private:
        bool initialize(VkPhysicalDevice &pDevice, VkDevice &device,
                        size_t max_allocation_size);
        void cleanup();
        VmaAllocator m_allocator;
        VkDevice m_device;
        VkPhysicalDevice m_physical_device;
        std::shared_ptr<QueueManager> m_queue_manager{nullptr};

	};

    class Buffer
    {
      public:
        static std::shared_ptr<Buffer> create(std::shared_ptr<MemoryManager> &device, size_t size,
                                              VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                                              VmaAllocationCreateFlags flags = 1);

        Buffer(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
               VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags = 1);
        ~Buffer();

        VkBuffer getBuffer() const;
        VmaAllocation getAllocation() const;

        void *getdPtr()
        {
            void *dPtr = malloc(64);
            return dPtr;
        }

        void *getPtr()
        {
            void* hHostPtr = nullptr;
            if (m_allocation_info.pMappedData == nullptr)
            {
                m_memory_manager->mapMemory(m_buffer, m_allocation, &hHostPtr);
                m_memory_manager->getVmaAllocationInfo(m_allocation, &m_allocation_info);
            }
            else
            {
                return m_allocation_info.pMappedData;
            }
            return hHostPtr;
        }

        void copyDataFrom(void *src, size_t size, size_t dst_offset = 0, size_t src_offset = 0,
                          uint32_t dst_access_flag = VK_ACCESS_SHADER_READ_BIT, uint32_t src_access_flag = VK_ACCESS_HOST_WRITE_BIT);
        void copyDataFrom(std::shared_ptr<Buffer> src, size_t size, size_t dst_offset = 0, size_t src_offset = 0);
        void copyDataTo(void *dst, size_t size, size_t src_offset = 0, size_t dst_offset = 0,
                        uint32_t src_access_flag = VK_ACCESS_SHADER_WRITE_BIT, uint32_t dst_access_flag = VK_ACCESS_HOST_READ_BIT);
        void copyDataTo(std::shared_ptr<Buffer> dst, size_t size, size_t dst_offset = 0, size_t src_offset = 0);
        VkMemoryPropertyFlags getMemoryPropertyFlags() const
        {
            return m_memory_property_flags;
        }

        VkDescriptorBufferInfo* getBufferInfo() 
        {
            return &m_write_descriptor_set;
        }    

        VkDescriptorType getDescriptorType() const
        {
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }

      protected:
        virtual void initialize(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
                                VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags = 1);
        virtual void cleanup();
        VkBuffer m_buffer{VK_NULL_HANDLE};
        VmaAllocation m_allocation{VK_NULL_HANDLE};
        VmaAllocationInfo m_allocation_info{};
        VkMemoryPropertyFlags m_memory_property_flags{0};
        VkDescriptorBufferInfo m_write_descriptor_set{};
        std::shared_ptr<MemoryManager> &m_memory_manager;
        std::vector<VkBufferMemoryBarrier> m_buffer_memory_barriers;
    };

    class SparseBuffer : public Buffer
    {
      public:
        static std::shared_ptr<SparseBuffer> create(std::shared_ptr<MemoryManager> &device, size_t size,
                                                    VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
                                                    VmaAllocationCreateFlags flags = 1);
        SparseBuffer(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
                     VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags = 1);

      private:
        void initialize(std::shared_ptr<MemoryManager> &mem_mamanger, size_t size, VkBufferUsageFlags usage,
                        VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags = 1) override;
        void cleanup() override;
        VkQueue m_queue;
    };

    
} // namespace runtime


#endif // MEMORY_H