#pragma once
#include "spirv_reflect.h"
#include <bitset>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

const int MAX_COUNT = 32;
namespace vkrt {
class Device;
class StorageBuffer;

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

private:
  struct DescriptorLayoutHash {
    std::size_t operator()(const DescriptorLayoutInfo &k) const {
      return k.hash();
    }
  };

  std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout,
                     DescriptorLayoutHash>
      layoutCache;
};

} // namespace vkrt