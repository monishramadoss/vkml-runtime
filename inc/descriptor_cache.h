#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <vector>
#include <memory>

namespace runtime{
class DescriptorLayoutCache
{
  public:
    void destory(VkDevice &device)
    {
        for (auto &pair : layoutCache)
            vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
    }

    VkDescriptorSetLayout create_descriptor_layout(VkDevice &device, uint32_t set_number,
                                                   VkDescriptorSetLayoutCreateInfo *info)
    {
        DescriptorLayoutInfo layoutinfo;
        layoutinfo.set_number = set_number;
        layoutinfo.bindings.reserve(info->bindingCount);
        bool isSorted = true;
        uint32_t lastBinding = 0;

        for (uint32_t i = 0; i < info->bindingCount; i++)
        {
            layoutinfo.bindings.push_back(info->pBindings[i]);
            if (info->pBindings[i].binding > lastBinding)
                lastBinding = info->pBindings[i].binding;
            else
                isSorted = false;
        }
        if (!isSorted)
            std::sort(
                layoutinfo.bindings.begin(), layoutinfo.bindings.end(),
                [](VkDescriptorSetLayoutBinding &a, VkDescriptorSetLayoutBinding &b) { return a.binding < b.binding; });

        auto it = layoutCache.find(layoutinfo);
        if (it != layoutCache.end())
            return (*it).second;

        VkDescriptorSetLayout layout;
        VkResult result = vkCreateDescriptorSetLayout(device, info, nullptr, &layout);
        if (result != VK_SUCCESS)
        {
            printf("failed to create descriptor set layout\n");
            return VK_NULL_HANDLE;
        }
        layoutCache[layoutinfo] = layout;
        return layout;
    }

    struct DescriptorLayoutInfo
    {
        // good idea to turn this into a inlined array
        std::vector<VkDescriptorSetLayoutBinding> bindings{};
        uint32_t set_number = UINT32_MAX;
        VkDescriptorSetLayoutCreateInfo create_info{};

        bool operator==(const DescriptorLayoutInfo &other) const
        {
            if (other.bindings.size() != bindings.size())
            {
                return false;
            }
            else
            {
                if (other.set_number != set_number)
                    return false;

                for (int i = 0; i < bindings.size(); i++)
                {
                    if (other.bindings[i].binding != bindings[i].binding)
                        return false;
                    if (other.bindings[i].descriptorType != bindings[i].descriptorType)
                        return false;
                    if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
                        return false;
                    if (other.bindings[i].stageFlags != bindings[i].stageFlags)
                        return false;
                }
                return true;
            }
        }

        size_t hash() const
        {
            using std::hash;
            using std::size_t;

            size_t result = hash<size_t>()(bindings.size());
            result ^= hash<uint32_t>()(set_number);
            for (const VkDescriptorSetLayoutBinding &b : bindings)
            {
                size_t binding_hash = static_cast<size_t>(b.binding) | static_cast<size_t>(b.descriptorType << 8) |
                                      static_cast<size_t>(b.descriptorCount) << 16 |
                                      static_cast<size_t>(b.stageFlags) << 24;
                result ^= hash<size_t>()(binding_hash);
            }

            return result;
        }
    };

  private:
    struct DescriptorLayoutHash
    {
        std::size_t operator()(const DescriptorLayoutInfo &k) const
        {
            return k.hash();
        }
    };

    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache{};
};

} // namespace runtime