#include <vulkan/vulkan.h>
#include <vector>


namespace runtime
{

class DescriptorAllocator
{
    VkDescriptorPool currentPool{nullptr};
    std::vector<VkDescriptorPool> usedPools;
    std::vector<VkDescriptorPool> freePools;
    std::vector<VkDescriptorPoolSize> pool_size;

    auto createPool(VkDevice &dev)
    {
        VkDescriptorPool pool;
        VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        uint32_t count = 0;
        for (auto &p : pool_size)
        {
            count += p.descriptorCount;
        }
        pool_info.flags = 0;
        pool_info.maxSets = count;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());
        pool_info.pPoolSizes = pool_size.data();

        vkCreateDescriptorPool(dev, &pool_info, nullptr, &pool);
        return pool;
    }

    auto grab_pool(VkDevice &device)
    {
        if (freePools.size() > 0)
        {
            auto pool = freePools.back();
            freePools.pop_back();
            usedPools.push_back(pool);
            return pool;
        }
        else
        {
            auto pool = createPool(device);
            usedPools.push_back(pool);
            return pool;
        }
    }

  public:
    DescriptorAllocator(const std::vector<VkDescriptorPoolSize> &pool_sizes) : pool_size(pool_sizes)
    {
    }

    DescriptorAllocator &operator=(const DescriptorAllocator &other)
    {
        currentPool = other.currentPool;
        usedPools = other.usedPools;
        freePools = other.freePools;
        pool_size = other.pool_size;

        return *this;
    }

    void reset_pools(VkDevice device)
    {
        for (auto p : usedPools)
        {
            vkResetDescriptorPool(device, p, 0);
            freePools.push_back(p);
        }
        usedPools.clear();
        currentPool = VK_NULL_HANDLE;
    }

    bool allocate(VkDevice &device, VkDescriptorSet *set, VkDescriptorSetLayout *layout)
    {
        if (currentPool == VK_NULL_HANDLE)
        {
            currentPool = grab_pool(device);
        }

        VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.pNext = nullptr;
        allocInfo.pSetLayouts = layout;
        allocInfo.descriptorPool = currentPool;
        allocInfo.descriptorSetCount = 1;

        VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
        bool needReallocate = false;

        switch (allocResult)
        {
        case VK_SUCCESS:
            return true;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            needReallocate = true;
            break;
        default:
            return false;
        }

        if (needReallocate)
        {
            currentPool = grab_pool(device);
            usedPools.push_back(currentPool);
            allocInfo.descriptorPool = currentPool;
            allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
            if (allocResult == VK_SUCCESS)
                return true;
        }

        return false;
    }

    void destory(VkDevice &device)
    {
        for (auto &pool : usedPools)
            vkDestroyDescriptorPool(device, pool, nullptr);
        for (auto &pool : freePools)
            vkDestroyDescriptorPool(device, pool, nullptr);
    }
};

}