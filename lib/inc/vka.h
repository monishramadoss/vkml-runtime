#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <spirv_reflect.h>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vk_mem_alloc.h>

void print_statistic(VmaStatistics stats)
{
    printf("\t\t blockCount: %i \n", stats.blockCount);
    printf("\t\t blockBytes: %lli \n", stats.blockBytes);
    printf("\t\t allocationCount: %i \n", stats.allocationCount);
    printf("\t\t allocationBytes: %lli \n", stats.allocationBytes);
}

void print_statistic(VmaDetailedStatistics stats)
{
    printf("\t unusedRangeCount: %i \n", stats.unusedRangeCount);
    printf("\t unusedRangeSizeMin: %lli \n", stats.unusedRangeSizeMin);
    printf("\t unusedRangeSizeMax %lli \n", stats.unusedRangeSizeMin);
    printf("\t allocationSizeMin: %lli \n", stats.allocationSizeMin);
    printf("\t allocationSizeMax: %lli \n", stats.allocationSizeMax);
    print_statistic(stats.statistics);
}

void print_statistic(VmaTotalStatistics stats)
{
    for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
    {
        printf("\t memoryType [%i]\n", i);
        print_statistic(stats.memoryType[i]);
    }
    for (int i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
    {
        printf("\t memoryHeap [%i]\n", i);
        print_statistic(stats.memoryHeap[i]);
    }
    printf("\t total\n");
    print_statistic(stats.total);
}

const int MAX_COUNT = 32;
<<<<<<< HEAD
namespace vkrt {
class Device;
class StorageBuffer;
=======
namespace vkrt
{
>>>>>>> 7fc3dd2 (seventh commit)

class queue {
  VkQueueFamilyProperties m_properties;
  std::vector<VkQueue> m_queues;
  std::vector<VkCommandPool> m_command_pools;
  std::unordered_map<size_t, std::vector<VkCommandBuffer>> m_command_buffers;
  std::vector<uint32_t> m_command_buffer_busy;
  std::vector<VkFence> m_fences;
  uint32_t m_idx;

  size_t m_queue_number = 0;

  void generate_command(VkDevice device, uint32_t idx) {
    VkCommandPoolCreateInfo poolInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.pNext = nullptr;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_idx;
    vkCreateCommandPool(device, &poolInfo, nullptr, &m_command_pools[idx]);
    VkCommandBufferAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = m_command_pools[idx];
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 32;
    m_command_buffers[idx].resize(allocInfo.commandBufferCount);
    vkAllocateCommandBuffers(device, &allocInfo, m_command_buffers[idx].data());
    m_command_buffer_busy[idx] = 0;
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fenceInfo, nullptr, &m_fences[idx]);
    ++m_queue_number;
  }

public:
  queue(uint32_t idx, VkDevice device, VkQueueFamilyProperties &properties)
      : m_properties(properties), m_idx(idx) {
    m_queues.resize(properties.queueCount);
    m_fences.resize(properties.queueCount);
    m_command_pools.resize(properties.queueCount, nullptr);
    m_command_buffer_busy.resize(properties.queueCount);
    vkGetDeviceQueue(device, idx, 0, &m_queues[0]);
    generate_command(device, 0);
  }

  bool isGraphicFamily() const {
    return m_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
  }
  bool isComputeFamily() const {
    return m_properties.queueFlags & VK_QUEUE_COMPUTE_BIT;
  }
  bool isTransferFamily() const {
    return m_properties.queueFlags & VK_QUEUE_TRANSFER_BIT;
  }
  bool isSparseBindingFamily() const {
    return m_properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
  }
  bool isVideoDecodeFamily() const {
    return m_properties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
  }
  bool isVideoEncodeFamily() const {
    return m_properties.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
  }
  bool isOtherFamily() const {
    return ~(VK_QUEUE_TRANSFER_BIT & VK_QUEUE_COMPUTE_BIT &
             VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_SPARSE_BINDING_BIT &
             VK_QUEUE_VIDEO_DECODE_BIT_KHR & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) &
           m_properties.queueFlags;
  }

  auto get_pool() { return m_command_pools[0]; }

  void find_free(const std::vector<uint32_t> &freeFlags, size_t qIdx,
                 uint32_t cIdx) {
    qIdx = 0;
    for (uint32_t flags : freeFlags) {
      if (flags != UINT32_MAX) {
        cIdx = log2<uint32_t>(flags & -flags) + 1;
        break;
      }
      ++qIdx;
    }
    cIdx = UINT32_MAX;
    qIdx = UINT64_MAX;
  }

  void get_cmd_buffer(VkCommandBuffer *cmd_buffer, size_t &qIdx,
                      uint32_t &cIdx) {
    find_free(m_command_buffer_busy, qIdx, cIdx);
    *cmd_buffer = m_command_buffers.at(qIdx).at(cIdx);
  }
  auto get_idx() const { return m_idx; }

  void run(size_t i) {
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = m_command_buffers.at(i).size();
    submitInfo.pCommandBuffers = m_command_buffers.at(i).data();
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pSignalSemaphores = nullptr;

    vkQueueSubmit(m_queues[i], 1, &submitInfo, m_fences[i]);
  }

  void wait(VkDevice device, size_t i) {
    vkWaitForFences(device, 1, &m_fences[i], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &m_fences[i]);
  }

  void wait(VkDevice device) {
    for (size_t i = 0; i < m_queue_number; ++i) {
      wait(device, i);
    }
  }

  void destroy(VkDevice device) {
    for (auto &fence : m_fences) {
      if (fence != nullptr)
        vkDestroyFence(device, fence, nullptr);
    }
    for (size_t i = 0; i < m_command_pools.size(); ++i) {
      if (m_command_pools[i] != nullptr) {
        vkFreeCommandBuffers(device, m_command_pools[i],
                             m_command_buffers[i].size(),
                             m_command_buffers[i].data());
        vkDestroyCommandPool(device, m_command_pools[i], nullptr);
      }
    }
  }
};

class DescriptorAllocator {
  VkDescriptorPool currentPool{nullptr};
  std::vector<VkDescriptorPool> usedPools;
  std::vector<VkDescriptorPool> freePools;
  std::vector<VkDescriptorPoolSize> pool_size;

  auto createPool(VkDevice &dev) {
    VkDescriptorPool pool;
    VkDescriptorPoolCreateInfo pool_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    uint32_t count = 0;
    for (auto &p : pool_size) {
      count += p.descriptorCount;
    }
    pool_info.flags = 0;
    pool_info.maxSets = count;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_size.size());
    pool_info.pPoolSizes = pool_size.data();

    vkCreateDescriptorPool(dev, &pool_info, nullptr, &pool);
    return pool;
  }

  auto grab_pool(VkDevice &device) {
    if (freePools.size() > 0) {
      auto pool = freePools.back();
      freePools.pop_back();
      usedPools.push_back(pool);
      return pool;
    } else {
      auto pool = createPool(device);
      usedPools.push_back(pool);
      return pool;
    }
  }

public:
  DescriptorAllocator(const std::vector<VkDescriptorPoolSize> &pool_sizes)
      : pool_size(pool_sizes) {}

  DescriptorAllocator &operator=(const DescriptorAllocator &other) {
    currentPool = other.currentPool;
    usedPools = other.usedPools;
    freePools = other.freePools;
    pool_size = other.pool_size;

    return *this;
  }

  void reset_pools(VkDevice device) {
    for (auto p : usedPools) {
      vkResetDescriptorPool(device, p, 0);
      freePools.push_back(p);
    }
    usedPools.clear();
    currentPool = VK_NULL_HANDLE;
  }

  bool allocate(VkDevice &device, VkDescriptorSet *set,
                VkDescriptorSetLayout *layout) {
    if (currentPool == VK_NULL_HANDLE) {
      currentPool = grab_pool(device);
      usedPools.push_back(currentPool);
    }

    VkDescriptorSetAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.pNext = nullptr;
    allocInfo.pSetLayouts = layout;
    allocInfo.descriptorPool = currentPool;
    allocInfo.descriptorSetCount = 1;

    VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
    bool needReallocate = false;

    switch (allocResult) {
    case VK_SUCCESS:
      return true;
    case VK_ERROR_FRAGMENTED_POOL:
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      needReallocate = true;
      break;
    default:
      return false;
    }

    if (needReallocate) {
      currentPool = grab_pool(device);
      usedPools.push_back(currentPool);
      allocInfo.descriptorPool = currentPool;
      allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
      if (allocResult == VK_SUCCESS)
        return true;
    }

    return false;
  }

  void destory(VkDevice &device) {
    reset_pools(device);
    for (auto &pool : usedPools)
      vkDestroyDescriptorPool(device, pool, nullptr);
    for (auto &pool : freePools)
      vkDestroyDescriptorPool(device, pool, nullptr);
  }
};

class DescriptorLayoutCache {
public:
  void destory(VkDevice &device) {
    for (auto pair : layoutCache)
      vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
  }

  VkDescriptorSetLayout
  create_descriptor_layout(VkDevice &device, uint32_t set_number,
                           VkDescriptorSetLayoutCreateInfo *info) {
    DescriptorLayoutInfo layoutinfo;
    layoutinfo.set_number = set_number;
    layoutinfo.bindings.reserve(info->bindingCount);
    bool isSorted = true;
    int lastBinding = -1;

    for (int i = 0; i < info->bindingCount; i++) {
      layoutinfo.bindings.push_back(info->pBindings[i]);
      if (info->pBindings[i].binding > lastBinding)
        lastBinding = info->pBindings[i].binding;
      else
        isSorted = false;
    }
    if (!isSorted)
      std::sort(
          layoutinfo.bindings.begin(), layoutinfo.bindings.end(),
          [](VkDescriptorSetLayoutBinding &a, VkDescriptorSetLayoutBinding &b) {
            return a.binding < b.binding;
          });

    auto it = layoutCache.find(layoutinfo);
    if (it != layoutCache.end())
      return (*it).second;

    VkDescriptorSetLayout layout;
    VkResult result =
        vkCreateDescriptorSetLayout(device, info, nullptr, &layout);
    if (result != VK_SUCCESS) {
      printf("failed to create descriptor set layout\n");
      return VK_NULL_HANDLE;
    }
    layoutCache[layoutinfo] = layout;
    return layout;
  }

  struct DescriptorLayoutInfo {
    // good idea to turn this into a inlined array
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    uint32_t set_number;
    VkDescriptorSetLayoutCreateInfo create_info;

    bool operator==(const DescriptorLayoutInfo &other) const {
      if (other.bindings.size() != bindings.size()) {
        return false;
      } else {
        if (other.set_number != set_number)
          return false;

        for (int i = 0; i < bindings.size(); i++) {
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

    size_t hash() const {
      using std::hash;
      using std::size_t;

      size_t result = hash<size_t>()(bindings.size());
      result ^= hash<uint32_t>()(set_number);
      for (const VkDescriptorSetLayoutBinding &b : bindings) {
        size_t binding_hash = b.binding | b.descriptorType << 8 |
                              b.descriptorCount << 16 | b.stageFlags << 24;
        result ^= hash<size_t>()(binding_hash);
      }

      return result;
    }
  };

<<<<<<< HEAD
private:
  struct DescriptorLayoutHash {
    std::size_t operator()(const DescriptorLayoutInfo &k) const {
      return k.hash();
=======
class DescriptorLayoutCache
{
  public:
    void destory(VkDevice &device)
    {
        for (auto &pair : layoutCache)
            vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
>>>>>>> 7fc3dd2 (seventh commit)
    }
  };

<<<<<<< HEAD
  std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout,
                     DescriptorLayoutHash>
      layoutCache;
=======
    VkDescriptorSetLayout create_descriptor_layout(VkDevice &device, uint32_t set_number,
                                                   VkDescriptorSetLayoutCreateInfo *info)
    {
        DescriptorLayoutInfo layoutinfo;
        layoutinfo.set_number = set_number;
        layoutinfo.bindings.reserve(info->bindingCount);
        bool isSorted = true;
        int lastBinding = -1;

        for (int i = 0; i < info->bindingCount; i++)
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

class Device
{

    VkDevice m_dev;
    VkPipelineCache m_pipeline_cache;
    VkPhysicalDevice m_pDev;

    VmaAllocator m_allocator;
    std::vector<queue> m_queues;
    std::multimap<VkQueueFlags, uint32_t> m_queue_map;
    std::map<uint32_t, std::pair<VkBuffer, VmaAllocation>> m_buffers;
    std::map<uint32_t, std::pair<VkImage, VmaAllocation>> m_images;
    std::map<uint32_t, VkBufferView> m_buffer_views;
    std::map<uint32_t, VkImageView> m_image_views;
    std::map<uint32_t, VkShaderModule> m_shader_modules;
    std::map<uint32_t, VkPipelineLayout> m_pipeline_layout;
    std::map<uint32_t, VkPipeline> m_pipelines;
    DescriptorAllocator *m_desc_pool_alloc;
    DescriptorLayoutCache *m_desc_layout_cache;

    void setupQueues(std::vector<VkQueueFamilyProperties> &queueFamilies)
    {
        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            m_queues.emplace_back(i, m_dev, queueFamilies[i]);
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_GRAPHICS_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_COMPUTE_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_TRANSFER_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_SPARSE_BINDING_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_PROTECTED_BIT, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_VIDEO_DECODE_BIT_KHR, i));
            if (queueFamilies[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
                m_queue_map.emplace(std::make_pair(VK_QUEUE_VIDEO_ENCODE_BIT_KHR, i));
        }
    }

    size_t findQueueFamilies(std::vector<VkQueueFamilyProperties> &queueFamilies,
                             std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos) const
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, nullptr);
        queueFamilies.resize(queueFamilyCount);
        queueCreateInfos.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, queueFamilies.data());

        int i = 0, j = 0;

        for (const auto &queueFamily : queueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfo.queueCount = queueFamily.queueCount;
            float *priorites = new float[queueCreateInfo.queueCount];
            std::fill(priorites, priorites + queueCreateInfo.queueCount, 0.9f);
            queueCreateInfo.pQueuePriorities = priorites;
            queueCreateInfos[i++] = queueCreateInfo;
            j += queueFamily.queueCount;
        }
        return j;
    }

    struct features
    {
        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR};
    } m_feats;

    struct properties
    {
        VkPhysicalDeviceProperties2 device_properties_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        VkPhysicalDeviceSubgroupProperties subgroup_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        VkPhysicalDeviceVulkan11Properties device_vulkan11_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        VkPhysicalDeviceVulkan12Properties device_vulkan12_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        VkPhysicalDeviceVulkan13Properties device_vulkan13_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    } m_props;

    void getFeaturesAndProperties(VkPhysicalDevice &pd)
    {
        m_feats.features2.pNext = &m_feats.features11;
        m_feats.features11.pNext = &m_feats.features12;
        m_feats.features12.pNext = &m_feats.features13;
        m_feats.features13.pNext = &m_feats.coo_matrix_features;
        vkGetPhysicalDeviceFeatures2(pd, &m_feats.features2);

        m_props.device_properties_2.pNext = &m_props.device_vulkan11_properties;
        m_props.device_vulkan11_properties.pNext = &m_props.device_vulkan12_properties;
        m_props.device_vulkan12_properties.pNext = &m_props.device_vulkan13_properties;
        m_props.device_vulkan13_properties.pNext = &m_props.subgroup_properties;
        vkGetPhysicalDeviceProperties2(pd, &m_props.device_properties_2);
    }

    void allocate_buffer(const VkBufferCreateInfo *bufferCreateInfo, VmaAllocationCreateInfo *vmaAllocCreateInfo,
                         VkBuffer *buffer, VmaAllocation *allocation, VmaAllocationInfo *alloc_info,
                         uint32_t *buffer_id)
    {
        uint32_t memTypeIdx = 0;
        if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
            VK_SUCCESS)
        {
            vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
            CHECK_RESULT(
                vmaCreateBuffer(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, buffer, allocation, alloc_info),
                "failure to allocation buffer");
            *buffer_id = m_buffers.size();
            m_buffers.emplace(*buffer_id, std::make_pair(*buffer, *allocation));
        }
        else
        {
            printf("Failed to find memory type index\n");
        }
    }

    void map_buffer(VmaAllocation alloc, void **data) const
    {
        vmaMapMemory(m_allocator, alloc, data);
    }

    void unmap_buffer(VmaAllocation alloc) const
    {
        vmaUnmapMemory(m_allocator, alloc);
    }

    void destroy_buffer(VkBuffer buffer, VmaAllocation allocation) const
    {
        vmaDestroyBuffer(m_allocator, buffer, allocation);
    }

    void allocate_image(const VkImageCreateInfo *imageCreateInfo, VmaAllocationCreateInfo *vmaAllocCreateInfo,
                        VkImage *image, VmaAllocation *allocation, VmaAllocationInfo *alloc_info, uint32_t *buffer_id)
    {
        uint32_t memTypeIdx = 0;
        if (vmaFindMemoryTypeIndexForImageInfo(m_allocator, imageCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
            VK_SUCCESS)
        {
            vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
            CHECK_RESULT(
                vmaCreateImage(m_allocator, imageCreateInfo, vmaAllocCreateInfo, image, allocation, alloc_info),
                "failure to allocate image");
            *buffer_id = m_images.size();
            m_images.emplace(*buffer_id, std::make_pair(*image, *allocation));
        }
        else
        {
            printf("Failed to find memory type index\n");
        }
    }

    void destroy_image(VkImage image, VmaAllocation allocation) const
    {
        vmaDestroyImage(m_allocator, image, allocation);
    }

    void allocate_buffer_view(const VkBufferViewCreateInfo *bufferViewCreateInfo, VkBufferView *bufferView,
                              uint32_t *buffer_id)
    {
        *buffer_id = m_buffer_views.size();
        CHECK_RESULT(vkCreateBufferView(m_dev, bufferViewCreateInfo, nullptr, bufferView),
                     "failure to allocate buffer view");
        m_buffer_views.emplace(*buffer_id, *bufferView);
    }

    void destroy_buffer_view(VkBufferView bufferView) const
    {
        vkDestroyBufferView(m_dev, bufferView, nullptr);
    }

    void allocate_image_view(const VkImageViewCreateInfo *imageViewCreateInfo, VkImageView *imageView,
                             uint32_t *buffer_id)
    {
        *buffer_id = m_image_views.size();
        CHECK_RESULT(vkCreateImageView(m_dev, imageViewCreateInfo, nullptr, imageView),
                     "failure to allocate image view");
        m_image_views.emplace(*buffer_id, *imageView);
    }

    void destroy_image_view(VkImageView imageView) const
    {
        vkDestroyImageView(m_dev, imageView, nullptr);
    }

    auto get_shader_module(const std::vector<uint32_t> &code, VkShaderModule *shaderModule, uint32_t *shader_id)
    {
        *shader_id = m_shader_modules.size();
        VkShaderModuleCreateInfo moduleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleCreateInfo.pCode = code.data();
        moduleCreateInfo.codeSize = code.size() * sizeof(uint32_t);
        moduleCreateInfo.flags = 0;
        moduleCreateInfo.pNext = nullptr;
        CHECK_RESULT(vkCreateShaderModule(m_dev, &moduleCreateInfo, nullptr, shaderModule),
                     " failed to create shaderModule");
        m_shader_modules.emplace(*shader_id, *shaderModule);
    }

    void destroy_shader_module(VkShaderModule shaderModule) const
    {
        vkDestroyShaderModule(m_dev, shaderModule, nullptr);
    }

    void get_pipeline_layout(VkPipelineLayoutCreateInfo *layoutInfo, VkPipelineLayout *layout, uint32_t *layout_id)
    {
        *layout_id = m_pipeline_layout.size();
        vkCreatePipelineLayout(m_dev, layoutInfo, nullptr, layout);
        m_pipeline_layout.emplace(*layout_id, *layout);
    }

    void destroy_pipeline_layout(VkPipelineLayout pipelineLayout) const
    {
        vkDestroyPipelineLayout(m_dev, pipelineLayout, nullptr);
    }

    void get_pipeline(VkComputePipelineCreateInfo *stageInfo, VkPipeline *pipeline, uint32_t *pipeline_id)
    {
        *pipeline_id = m_pipelines.size();
        vkCreateComputePipelines(m_dev, m_pipeline_cache, 1, stageInfo, nullptr, pipeline);
        m_pipelines.emplace(*pipeline_id, *pipeline);
    }

    void destroy_pipeline(VkPipeline pipeline) const
    {
        vkDestroyPipeline(m_dev, pipeline, nullptr);
    }

    template <uint32_t x, uint32_t y, uint32_t z> friend class Program;
    friend class StorageBuffer;

  public:
    Device(const Device &other)
    {
        m_dev = other.m_dev;
        m_pipeline_cache = other.m_pipeline_cache;
        m_pDev = other.m_pDev;
        m_allocator = other.m_allocator;
        m_queues = other.m_queues;
        m_buffers = other.m_buffers;
        m_images = other.m_images;
        m_buffer_views = other.m_buffer_views;
        m_image_views = other.m_image_views;
        m_feats = other.m_feats;
        m_props = other.m_props;
        m_desc_pool_alloc = other.m_desc_pool_alloc;
        m_desc_layout_cache = other.m_desc_layout_cache;
    }

    Device &operator=(const Device &other)
    {
        m_dev = other.m_dev;
        m_pipeline_cache = other.m_pipeline_cache;
        m_pDev = other.m_pDev;
        m_allocator = other.m_allocator;
        m_queues = other.m_queues;
        m_buffers = other.m_buffers;
        m_images = other.m_images;
        m_buffer_views = other.m_buffer_views;
        m_image_views = other.m_image_views;
        m_feats = other.m_feats;
        m_props = other.m_props;
        m_desc_pool_alloc = other.m_desc_pool_alloc;
        m_desc_layout_cache = other.m_desc_layout_cache;
        return *this;
    }

    Device(VkInstance &inst, VkPhysicalDevice &pd) : m_pDev(pd)
    {
        m_desc_pool_alloc = new DescriptorAllocator({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}});
        m_desc_layout_cache = new DescriptorLayoutCache;
        getFeaturesAndProperties(pd);
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, queueFamilies.data());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        size_t max_queues = findQueueFamilies(queueFamilies, queueCreateInfos);
        printf("Device has %zi queue families\n", max_queues);

        std::vector<const char *> deviceExtensions;
        if (m_feats.coo_matrix_features.cooperativeMatrix)
        {
            deviceExtensions.push_back(VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME);
        }

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceInfo.enabledExtensionCount = deviceExtensions.size();
        deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

        vkCreateDevice(pd, &deviceInfo, nullptr, &m_dev);
        volkLoadDevice(m_dev);
        setupQueues(queueFamilies);

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        vkCreatePipelineCache(m_dev, &pipelineCacheCreateInfo, nullptr, &m_pipeline_cache);

        VolkDeviceTable dt;
        volkLoadDeviceTable(&dt, m_dev);

        VmaVulkanFunctions vmaFuncs = {};
        vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        vmaFuncs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vmaFuncs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vmaFuncs.vkAllocateMemory = dt.vkAllocateMemory;
        vmaFuncs.vkFreeMemory = dt.vkFreeMemory;
        vmaFuncs.vkMapMemory = dt.vkMapMemory;
        vmaFuncs.vkUnmapMemory = dt.vkUnmapMemory;
        vmaFuncs.vkFlushMappedMemoryRanges = dt.vkFlushMappedMemoryRanges;
        vmaFuncs.vkInvalidateMappedMemoryRanges = dt.vkInvalidateMappedMemoryRanges;
        vmaFuncs.vkBindBufferMemory = dt.vkBindBufferMemory;
        vmaFuncs.vkBindImageMemory = dt.vkBindImageMemory;
        vmaFuncs.vkGetBufferMemoryRequirements = dt.vkGetBufferMemoryRequirements;
        vmaFuncs.vkGetImageMemoryRequirements = dt.vkGetImageMemoryRequirements;
        vmaFuncs.vkCreateBuffer = dt.vkCreateBuffer;
        vmaFuncs.vkDestroyBuffer = dt.vkDestroyBuffer;
        vmaFuncs.vkCreateImage = dt.vkCreateImage;
        vmaFuncs.vkDestroyImage = dt.vkDestroyImage;
        vmaFuncs.vkCmdCopyBuffer = dt.vkCmdCopyBuffer;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = pd;
        allocatorInfo.device = m_dev;
        allocatorInfo.pVulkanFunctions = &vmaFuncs;
        allocatorInfo.instance = inst;
        allocatorInfo.preferredLargeHeapBlockSize = static_cast<VkDeviceSize>(1) << 28;
        allocatorInfo.vulkanApiVersion = volkGetInstanceVersion();
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

        CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator), "Failed to create allocator");
    }

    void destory()
    {
        for (auto &id_buf : m_buffers)
            destroy_buffer(id_buf.second.first, id_buf.second.second);
        for (auto &id_img : m_images)
            destroy_image(id_img.second.first, id_img.second.second);
        for (auto &id_buf_view : m_buffer_views)
            destroy_buffer_view(id_buf_view.second);
        for (auto &id_img_view : m_image_views)
            destroy_image_view(id_img_view.second);
        for (auto &id_shader_module : m_shader_modules)
            destroy_shader_module(id_shader_module.second);
        for (auto &id_pipeline_layout : m_pipeline_layout)
            destroy_pipeline_layout(id_pipeline_layout.second);
        for (auto &id_pipeline : m_pipelines)
            destroy_pipeline(id_pipeline.second);
        for (auto &q : m_queues)
            q.destroy(m_dev);
        m_desc_layout_cache->destory(m_dev);
        m_desc_pool_alloc->destory(m_dev);
        delete m_desc_layout_cache;
        delete m_desc_pool_alloc;
        vmaDestroyAllocator(m_allocator);
        vkDestroyPipelineCache(m_dev, m_pipeline_cache, nullptr);
        vkDestroyDevice(m_dev, nullptr);
    }

    auto getMemoryProperty(VmaAllocation &alloc) const
    {
        VkMemoryPropertyFlags flags;
        vmaGetAllocationMemoryProperties(m_allocator, alloc, &flags);
        return flags;
    }

    auto flushAllocation(VmaAllocation &alloc, size_t offset, size_t size) const
    {
        CHECK_RESULT(vmaFlushAllocation(m_allocator, alloc, offset, size), "failed to flush memory allocation");
    }

    auto getDescriptorSet(uint32_t set_number, VkDescriptorSetLayoutCreateInfo *create_info,
                          std::vector<VkWriteDescriptorSet> &writes, VkDescriptorSet &set,
                          VkDescriptorSetLayout &layout)
    {
        layout = m_desc_layout_cache->create_descriptor_layout(m_dev, set_number, create_info);
        bool success = m_desc_pool_alloc->allocate(m_dev, &set, &layout);
        if (!success)
            return false;
        for (VkWriteDescriptorSet &w : writes)
            w.dstSet = set;
        vkUpdateDescriptorSets(m_dev, writes.size(), writes.data(), 0, nullptr);
        return true;
    }

    void getCmdBuffer(VkShaderStageFlagBits shaderFlag, VkCommandBuffer *cmdBuffer)
    {
        switch (shaderFlag)
        {
        case VK_SHADER_STAGE_COMPUTE_BIT:
            auto tmp = m_queue_map.equal_range(VK_QUEUE_COMPUTE_BIT);
            size_t q_idx = 0;
            uint32_t c_idx = 0;
            m_queues[tmp.first->second].get_cmd_buffer(cmdBuffer, q_idx, c_idx);
            break;
        }
    }
};

class StorageBuffer
{
  protected:
    VkBuffer m_buffer = nullptr;
    VkImage m_image = nullptr;
    VkBufferView m_buffer_view = nullptr;
    VkImageView m_image_view = nullptr;

    VmaAllocation m_allocation = nullptr;
    VmaAllocationInfo m_allocation_info = {};

    VkBufferCreateInfo m_buffer_create_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VkImageCreateInfo m_image_create_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    VkBufferViewCreateInfo m_buffer_view_create_info{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    VkImageViewCreateInfo m_image_view_create_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

    VmaAllocationCreateInfo m_allocation_create_info;

    uint32_t m_buffer_id = UINT32_MAX;
    Device *m_dev = nullptr;
    int m_flags = 0;
    bool m_is_mapped = false;
    void *m_data = nullptr;
    union DescriptorInfo {
        VkDescriptorBufferInfo desc_buffer_info;
        VkDescriptorImageInfo desc_image_info;
    } m_desc_info;

    enum class BUFFERTYPE
    {
        BUFFER,
        IMAGE,
        BUFFER_VIEW,
        IMAGE_VIEW
    } m_type;

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
        m_allocation_create_info.priority = 1.0f;
        m_allocation_create_info.pool = nullptr;
        m_allocation_create_info.requiredFlags = 0;
        m_allocation_create_info.preferredFlags = 0;
        m_allocation_create_info.memoryTypeBits = UINT32_MAX;
    }
    void map()
    {
        if (m_is_mapped && m_allocation_info.pMappedData != nullptr)
        {
            m_data = m_allocation_info.pMappedData;
        }
        else if (m_is_mapped)
        {
            m_dev->map_buffer(m_allocation, &m_data);
        }
    }

    void lazy_load()
    {
        if (m_type == BUFFERTYPE::BUFFER && m_buffer == nullptr)
        {
            m_dev->allocate_buffer(&m_buffer_create_info, &m_allocation_create_info, &m_buffer, &m_allocation,
                                   &m_allocation_info, &m_buffer_id);
            map();
        }
        if (m_type == BUFFERTYPE::IMAGE && m_image == nullptr)
        {
            m_dev->allocate_image(&m_image_create_info, &m_allocation_create_info, &m_image, &m_allocation,
                                  &m_allocation_info, &m_buffer_id);
            map();
        }
        if (m_type == BUFFERTYPE::BUFFER_VIEW && m_buffer_view == nullptr)
            m_dev->allocate_buffer_view(&m_buffer_view_create_info, &m_buffer_view, &m_buffer_id);
        if (m_type == BUFFERTYPE::IMAGE_VIEW && m_image_view == nullptr)
            m_dev->allocate_image_view(&m_image_view_create_info, &m_image_view, &m_buffer_id);
    }

  public:
    StorageBuffer(Device &dev, size_t size, VkBufferUsageFlags flags, bool is_mapped = false)
        : m_flags(static_cast<int>(flags)), m_is_mapped(is_mapped), m_type(BUFFERTYPE::BUFFER)
    {
        if (size >= 1llu << 32U)
            printf("ERROR IN ALLOCATION OF DATA");
        m_dev = &dev;
        m_buffer_create_info.pNext = nullptr;
        m_buffer_create_info.size = size;
        m_buffer_create_info.usage = flags;
        m_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        m_allocation_create_info.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
        m_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        m_type = BUFFERTYPE::BUFFER;
        setFlags(static_cast<int>(flags));
        lazy_load();
    }

    StorageBuffer(Device &dev, VkImageUsageFlags flags, uint32_t mipLevels, VkImageType imageType, VkFormat format,
                  VkExtent3D extent, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkImageTiling tiling)
        : m_flags(static_cast<int>(flags)), m_type(BUFFERTYPE::IMAGE)
    {
        m_dev = &dev;
        m_image_create_info.pNext = nullptr;
        m_image_create_info.imageType = imageType;
        m_image_create_info.format = format;
        m_image_create_info.extent = extent;
        m_image_create_info.mipLevels = mipLevels;
        m_image_create_info.arrayLayers = arrayLayers;
        m_image_create_info.samples = samples;
        m_image_create_info.tiling = tiling;
        m_image_create_info.usage = flags;
        m_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        m_allocation_create_info.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
        m_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        setFlags(static_cast<int>(flags));
    }

    StorageBuffer(Device &dev, StorageBuffer &buffer, VkImageViewCreateInfo &imageViewCreateInfo)
        : m_type(BUFFERTYPE::IMAGE_VIEW), m_desc_info{}, m_allocation_create_info{}
    {
        m_dev = &dev;
        m_image_view_create_info = imageViewCreateInfo;
        m_image_view_create_info.image = buffer.m_image;
    }

    StorageBuffer(Device &dev, StorageBuffer &buffer, VkBufferViewCreateInfo &bufferViewCreateInfo)
        : m_type(BUFFERTYPE::BUFFER_VIEW), m_desc_info{}, m_allocation_create_info{}
    {
        m_dev = &dev;
        m_buffer_view_create_info = bufferViewCreateInfo;
        m_buffer_view_create_info.buffer = buffer.m_buffer;
    }

    StorageBuffer &operator=(const StorageBuffer &other)
    {
        m_dev = other.m_dev;
        m_buffer = other.m_buffer;
        m_allocation = other.m_allocation;
        m_allocation_info = other.m_allocation_info;
        m_buffer_create_info = other.m_buffer_create_info;
        m_allocation_create_info = other.m_allocation_create_info;
        m_buffer_id = other.m_buffer_id;
        m_flags = other.m_flags;
        m_desc_info = other.m_desc_info;
        m_type = other.m_type;
        return *this;
    }

    StorageBuffer(const StorageBuffer &other)
    {
        m_dev = other.m_dev;
        m_buffer = other.m_buffer;
        m_allocation = other.m_allocation;
        m_allocation_info = other.m_allocation_info;
        m_buffer_create_info = other.m_buffer_create_info;
        m_allocation_create_info = other.m_allocation_create_info;
        m_buffer_id = other.m_buffer_id;
        m_flags = other.m_flags;
        m_desc_info = other.m_desc_info;
        m_type = other.m_type;
    }

    auto getMemoryProperties()
    {
        VkMemoryPropertyFlagBits memFlags{};
        return m_dev->getMemoryProperty(m_allocation);
    }

    auto getSize() const
    {
        return m_buffer_create_info.size;
    }

    void *data()
    {
        return m_data;
    }

    auto GetAccessFlags() const
    {
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            return VK_ACCESS_TRANSFER_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
            return VK_ACCESS_SHADER_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
            return VK_ACCESS_SHADER_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            return VK_ACCESS_UNIFORM_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            return VK_ACCESS_SHADER_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
            return VK_ACCESS_INDEX_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
            return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            return VK_ACCESS_SHADER_READ_BIT;
        if (m_buffer_create_info.usage & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR)
            return VK_ACCESS_SHADER_READ_BIT;
        return VK_ACCESS_HOST_WRITE_BIT;
    }

    bool isLive(void) const
    {
        return m_buffer_id != UINT32_MAX;
    }

    void setWriteDescriptorSet(VkWriteDescriptorSet &write)
    {
        lazy_load();
        if (m_buffer != nullptr && m_type == BUFFERTYPE::BUFFER)
        {
            m_desc_info.desc_buffer_info.buffer = m_buffer;
            m_desc_info.desc_buffer_info.offset = 0;
            m_desc_info.desc_buffer_info.range = m_allocation_info.size;
            write.pBufferInfo = &(m_desc_info.desc_buffer_info);
        }
        else if (m_image != nullptr && (m_type == BUFFERTYPE::IMAGE || m_type == BUFFERTYPE::IMAGE_VIEW))
        {
            m_desc_info.desc_image_info.imageLayout = m_image_create_info.initialLayout;
            if (m_type == BUFFERTYPE::IMAGE_VIEW)
                m_desc_info.desc_image_info.imageView = m_image_view;
            write.pImageInfo = &(m_desc_info.desc_image_info);
        }
    }
};

template <typename T> class SendBuffer : public StorageBuffer
{
  public:
    SendBuffer(Device &dev, const std::vector<T> &data, const StorageBuffer &outputBuffer)
        : StorageBuffer(dev, data.size() * sizeof T, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true)
    {
        auto memProps = dev.getMemoryProperty(m_allocation);
        memcpy(dataPtr(), data.data(), data.size() * sizeof T);
        if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            if (getMemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                m_dev->flushAllocation(m_allocation, 0, sizeof(T) * data.size());

            VkBufferMemoryBarrier bufMemBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            bufMemBarrier.srcAccessMask = GetAccessFlags();
            bufMemBarrier.dstAccessMask = outputBuffer.GetAccessFlags();
            bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.buffer = m_buffer;
            bufMemBarrier.offset = 0;
            bufMemBarrier.size = VK_WHOLE_SIZE;
        }
    }
};

template <typename T> class RecvBuffer : public StorageBuffer
{
  public:
    RecvBuffer(Device &dev, std::vector<T> &data, StorageBuffer &inputBuffer)
        : StorageBuffer(dev, inputBuffer.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT, true)
    {
        auto memProps = dev.getMemoryProperty(m_allocation);
        if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            if (getMemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                m_dev->flushAllocation(m_allocation, 0, sizeof(T) * data.size());

            VkBufferMemoryBarrier bufMemBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            bufMemBarrier.srcAccessMask = inputBuffer.GetAccessFlags();
            bufMemBarrier.dstAccessMask = GetAccessFlags();
            bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufMemBarrier.buffer = m_buffer;
            bufMemBarrier.offset = 0;
            bufMemBarrier.size = VK_WHOLE_SIZE;
        }

        memcpy_s(data.data(), data.size() * sizeof T, dataPtr(), inputBuffer.getSize());
    }
};

template <uint32_t x = 0, uint32_t y = 0, uint32_t z = 0> class Program
{
    VkFence *m_fence = nullptr;
    VkSubmitInfo m_submit_info = {};
    VkCommandBuffer m_cmd = nullptr;
    VkPipeline m_pipeline = nullptr;
    VkPipelineLayout m_pipeline_layout = nullptr;

    VkShaderModule m_module;
    SpvReflectShaderModule m_spv_module = {};
    struct set
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};
        std::vector<VkWriteDescriptorSet> writes{};
        VkDescriptorSetLayout layout = nullptr;
        VkDescriptorSet descriptorSet = nullptr;
    };

    std::vector<struct set> m_sets;
    Device *m_dev = nullptr;
    uint32_t m_program_id = UINT32_MAX;
    uint32_t m_pipeline_layout_id = UINT32_MAX;
    uint32_t m_pipeline_id = UINT32_MAX;

  public:
    Program(Device &dev, const std::vector<uint32_t> &code, std::vector<StorageBuffer> &bufs)
    {
        m_dev = &dev;
        m_dev->get_shader_module(code, &m_module, &m_program_id);

        SpvReflectResult result =
            spvReflectCreateShaderModule(code.size() * sizeof(uint32_t), code.data(), &m_spv_module);
        uint32_t count = 0;
        result = spvReflectEnumerateDescriptorSets(&m_spv_module, &count, NULL);
        std::vector<SpvReflectDescriptorSet *> sets(count);
        m_sets.resize(count);
        result = spvReflectEnumerateDescriptorSets(&m_spv_module, &count, sets.data());
        size_t l = 0;
        for (size_t i = 0; i < sets.size(); ++i)
        {
            const SpvReflectDescriptorSet &refl_set = *(sets[i]);
            auto &set = m_sets[i];
            set.bindings.resize(refl_set.binding_count);
            set.writes.resize(refl_set.binding_count);
            for (size_t j = 0; j < refl_set.binding_count; ++j)
            {
                auto &binding = set.bindings[j];
                const SpvReflectDescriptorBinding &refl_binding = *(refl_set.bindings[j]);
                binding.binding = refl_binding.binding;
                binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
                binding.descriptorCount = 1;
                for (uint32_t k = 0; k < refl_binding.array.dims_count; ++k)
                    binding.descriptorCount *= refl_binding.array.dims[k];
                binding.stageFlags = static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage);

                auto &write = set.writes[j];
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.pNext = nullptr;
                write.descriptorCount = binding.descriptorCount;
                write.descriptorType = binding.descriptorType;
                write.dstBinding = binding.binding;
                bufs[l++].setWriteDescriptorSet(write);
            }
            VkDescriptorSetLayoutCreateInfo create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            create_info.bindingCount = static_cast<uint32_t>(set.bindings.size());
            create_info.pBindings = set.bindings.data();
            m_dev->getDescriptorSet(i, &create_info, set.writes, set.descriptorSet, set.layout);
        }

        VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stageInfo.pNext = nullptr;
        stageInfo.stage = static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage);
        stageInfo.module = m_module;
        stageInfo.pName = m_spv_module.entry_point_name;
        m_dev->getCmdBuffer(stageInfo.stage, &m_cmd);
        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layoutInfo.pNext = nullptr;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(m_sets.size());
        std::vector<VkDescriptorSetLayout> layouts(m_sets.size());
        for (size_t i = 0; i < m_sets.size(); ++i)
            layouts[i] = m_sets[i].layout;
        layoutInfo.pSetLayouts = layouts.data();
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        m_dev->get_pipeline_layout(&layoutInfo, &m_pipeline_layout, &m_pipeline_layout_id);

        switch (static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage))
        {
        case VK_SHADER_STAGE_COMPUTE_BIT:
            VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
            pipelineInfo.pNext = nullptr;
            pipelineInfo.stage = stageInfo;
            pipelineInfo.layout = m_pipeline_layout;
            m_dev->get_pipeline(&pipelineInfo, &m_pipeline, &m_pipeline_id);
            break;
        };

        if (x != 0 && y != 0 && z != 0)
            record(x, y, z);
    }

    void record(uint32_t x, uint32_t y, uint32_t z)
    {
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(m_cmd, &beginInfo);
        vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        for (auto &set : m_sets)
            vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &set.descriptorSet,
                                    0, nullptr);

        vkCmdDispatch(m_cmd, x, y, z);
        vkEndCommandBuffer(m_cmd);
    }
>>>>>>> 7fc3dd2 (seventh commit)
};

} // namespace vkrt