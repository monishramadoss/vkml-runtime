#include <vk_mem_alloc.h>

enum class StorageTYPE
{
    BUFFER,
    IMAGE,
    BUFFER_VIEW,
    IMAGE_VIEW
};

namespace runtime
{

class buffer
{
  protected:
    StorageTYPE m_type{StorageTYPE::BUFFER};
    std::shared_ptr<vkrt_buffer> m_storage;
    Device *m_dev = nullptr;

    int m_flags = 0;
    bool m_is_mapped = false;
    void *m_data = nullptr;
    VkDescriptorBufferInfo desc_buffer_info;
    VkBufferCreateInfo m_create_info;
    VmaAllocationCreateInfo m_allocation_create_info;

    void setFlags(int flags)
    {
        if (flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            m_allocation_create_info.flags |=
                VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        else if (m_is_mapped || flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT ||
                 flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT || flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
            m_allocation_create_info.flags |=
                VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        else
            m_allocation_create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        m_allocation_create_info.priority = 0.9f;
        m_allocation_create_info.pool = nullptr;
        m_allocation_create_info.requiredFlags = 0;
        m_allocation_create_info.preferredFlags = 0;
        m_allocation_create_info.memoryTypeBits = UINT32_MAX;
    }

    void map()
    {
        if (m_is_mapped && m_storage->allocation_info.pMappedData != nullptr)
        {
            m_data = m_storage->allocation_info.pMappedData;
        }
        else if (m_is_mapped)
        {
            m_dev->map_buffer(m_storage->allocation, &m_data);
        }
    }

    void lazy_load()
    {

        m_storage = m_dev->construct(&m_create_info, &m_allocation_create_info);
        if (isLive())
            map();     
       
    }

  public:
    buffer(Device &dev, size_t size, VkBufferUsageFlags flags, bool is_mapped = false)
        : m_flags(static_cast<int>(flags)), m_is_mapped(is_mapped), m_type(StorageTYPE::BUFFER)
    {
        m_storage = nullptr;
        m_dev = &dev;
        m_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        m_create_info.pNext = nullptr;
        m_create_info.size = size;
        m_create_info.flags = 0;
        m_create_info.usage = flags;
        m_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        m_create_info.pQueueFamilyIndices = 0;
        m_create_info.queueFamilyIndexCount = 0;
        m_allocation_create_info.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
        m_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        setFlags(static_cast<int>(flags));
    }

    buffer &operator=(const buffer &other)
    {
        m_dev = other.m_dev;
        m_storage = other.m_storage;        
        m_create_info = other.m_create_info;
        m_allocation_create_info = other.m_allocation_create_info;
        m_flags = other.m_flags;
        desc_buffer_info = other.desc_buffer_info;
        m_type = other.m_type;
        return *this;
    }

    buffer(const buffer &other)
    {
        m_dev = other.m_dev;
        m_storage = other.m_storage;
        m_create_info = other.m_create_info;
        m_allocation_create_info = other.m_allocation_create_info;
        m_flags = other.m_flags;
        desc_buffer_info = other.desc_buffer_info;
        m_type = other.m_type;
    }

    auto getSize() const {
        return m_create_info.size;
    }

    void upload(void *src, size_t size, size_t offset = 0)
    {

        if (isLive() && m_dev->getMemoryProperty(m_storage->allocation) & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            m_dev->copyMemoryToBuffer(m_storage->allocation, src, size, offset);
        }
        else if (isLive())
        {
            buffer staging(*m_dev, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
            staging.upload(src, size, offset); 
            staging.lazy_load();
            //1 VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT;
            
            /*VkBufferCopy2 copy = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
            copy.pNext = nullptr;
            copy.srcOffset = staging.m_storage->allocation_info.offset;
            if (!isLive())
                lazy_load();
            copy.dstOffset = m_storage->allocation_info.offset;*/
            //2 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_BIT
        }
    }

    void download(void *dst, size_t size, size_t offset=0)
    {
        if (isLive() && m_dev->getMemoryProperty(m_storage->allocation) & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            m_dev->copyBufferToMemory(m_storage->allocation, dst, size, offset);
        }
        else
        {
            buffer staging(*m_dev, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
            //staging.download(dst, size, offset);
            //2 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT;       
            
          /*  VkBufferCopy2 copy = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
            copy.pNext = nullptr;
            copy.dstOffset = m_storage->allocation_info.offset;
            if (!isLive())
                lazy_load();
            copy.srcOffset = staging.m_storage->allocation_info.offset;*/
            //1 VK_PIPELINE_STAGE_COMPUTE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT

        }
    }

    bool isLive(void) const
    {
        return m_storage.use_count()  && m_storage->id != UINT64_MAX;
    };

};

} // namespace vkrt