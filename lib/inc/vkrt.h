#pragma once

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#ifdef DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "vka.h"

void print_statistic(VmaStatistics stats) {
  printf("\t\t blockCount: %i \n", stats.blockCount);
  printf("\t\t blockBytes: %lli \n", stats.blockBytes);
  printf("\t\t allocationCount: %i \n", stats.allocationCount);
  printf("\t\t allocationBytes: %lli \n", stats.allocationBytes);
}

void print_statistic(VmaDetailedStatistics stats) {
  printf("\t unusedRangeCount: %i \n", stats.unusedRangeCount);
  printf("\t unusedRangeSizeMin: %lli \n", stats.unusedRangeSizeMin);
  printf("\t unusedRangeSizeMax %lli \n", stats.unusedRangeSizeMin);
  printf("\t allocationSizeMin: %lli \n", stats.allocationSizeMin);
  printf("\t allocationSizeMax: %lli \n", stats.allocationSizeMax);
  print_statistic(stats.statistics);
}

void print_statistic(VmaTotalStatistics stats) {
  for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
    printf("\t memoryType [%i]\n", i);
    print_statistic(stats.memoryType[i]);
  }
  for (int i = 0; i < VK_MAX_MEMORY_HEAPS; ++i) {
    printf("\t memoryHeap [%i]\n", i);
    print_statistic(stats.memoryHeap[i]);
  }
  printf("\t total\n");
  print_statistic(stats.total);
}

namespace vkrt {
class StorageBuffer;
template <uint32_t x = 0, uint32_t y = 0, uint32_t z = 0> class Program;

class Device {

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

  void setupQueues(std::vector<VkQueueFamilyProperties> &queueFamilies) {
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
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

  size_t
  findQueueFamilies(std::vector<VkQueueFamilyProperties> &queueFamilies,
                    std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount,
                                             nullptr);
    queueFamilies.resize(queueFamilyCount);
    queueCreateInfos.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount,
                                             queueFamilies.data());

    int i = 0, j = 0;

    for (const auto &queueFamily : queueFamilies) {
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

  struct features {
    VkPhysicalDeviceFeatures2 features2 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features features11 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features features12 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features features13 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR};
  } m_feats;

  struct properties {
    VkPhysicalDeviceProperties2 device_properties_2 = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceSubgroupProperties subgroup_properties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    VkPhysicalDeviceVulkan11Properties device_vulkan11_properties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    VkPhysicalDeviceVulkan12Properties device_vulkan12_properties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    VkPhysicalDeviceVulkan13Properties device_vulkan13_properties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
  } m_props;

  void getFeaturesAndProperties(VkPhysicalDevice &pd) {
    m_feats.features2.pNext = &m_feats.features11;
    m_feats.features11.pNext = &m_feats.features12;
    m_feats.features12.pNext = &m_feats.features13;
    m_feats.features13.pNext = &m_feats.coo_matrix_features;
    vkGetPhysicalDeviceFeatures2(pd, &m_feats.features2);

    m_props.device_properties_2.pNext = &m_props.device_vulkan11_properties;
    m_props.device_vulkan11_properties.pNext =
        &m_props.device_vulkan12_properties;
    m_props.device_vulkan12_properties.pNext =
        &m_props.device_vulkan13_properties;
    m_props.device_vulkan13_properties.pNext = &m_props.subgroup_properties;
    vkGetPhysicalDeviceProperties2(pd, &m_props.device_properties_2);
  }

  void allocate_buffer(const VkBufferCreateInfo *bufferCreateInfo,
                       VmaAllocationCreateInfo *vmaAllocCreateInfo,
                       VkBuffer *buffer, VmaAllocation *allocation,
                       VmaAllocationInfo *alloc_info, uint32_t *buffer_id) {
    uint32_t memTypeIdx = 0;
    if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo,
                                            vmaAllocCreateInfo,
                                            &memTypeIdx) == VK_SUCCESS) {
      vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
      CHECK_RESULT(vmaCreateBuffer(m_allocator, bufferCreateInfo,
                                   vmaAllocCreateInfo, buffer, allocation,
                                   alloc_info),
                   "failure to allocation buffer");
      *buffer_id = m_buffers.size();
      m_buffers.emplace(*buffer_id, std::make_pair(*buffer, *allocation));
    } else {
      printf("Failed to find memory type index\n");
    }
  }

  void map_buffer(VmaAllocation alloc, void **data) {
    vmaMapMemory(m_allocator, alloc, data);
  }

  void unmap_buffer(VmaAllocation alloc) { vmaUnmapMemory(m_allocator, alloc); }

  void destroy_buffer(VkBuffer buffer, VmaAllocation allocation) {
    vmaDestroyBuffer(m_allocator, buffer, allocation);
  }

  void allocate_image(const VkImageCreateInfo *imageCreateInfo,
                      VmaAllocationCreateInfo *vmaAllocCreateInfo,
                      VkImage *image, VmaAllocation *allocation,
                      VmaAllocationInfo *alloc_info, uint32_t *buffer_id) {
    uint32_t memTypeIdx = 0;
    if (vmaFindMemoryTypeIndexForImageInfo(m_allocator, imageCreateInfo,
                                           vmaAllocCreateInfo,
                                           &memTypeIdx) == VK_SUCCESS) {
      vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
      CHECK_RESULT(vmaCreateImage(m_allocator, imageCreateInfo,
                                  vmaAllocCreateInfo, image, allocation,
                                  alloc_info),
                   "failure to allocate image");
      *buffer_id = m_images.size();
      m_images.emplace(*buffer_id, std::make_pair(*image, *allocation));
    } else {
      printf("Failed to find memory type index\n");
    }
  }

  void destroy_image(VkImage image, VmaAllocation allocation) {
    vmaDestroyImage(m_allocator, image, allocation);
  }

  void allocate_buffer_view(const VkBufferViewCreateInfo *bufferViewCreateInfo,
                            VkBufferView *bufferView, uint32_t *buffer_id) {
    *buffer_id = m_buffer_views.size();
    CHECK_RESULT(
        vkCreateBufferView(m_dev, bufferViewCreateInfo, nullptr, bufferView),
        "failure to allocate buffer view");
    m_buffer_views.emplace(*buffer_id, *bufferView);
  }

  void destroy_buffer_view(VkBufferView bufferView) {
    vkDestroyBufferView(m_dev, bufferView, nullptr);
  }

  void allocate_image_view(const VkImageViewCreateInfo *imageViewCreateInfo,
                           VkImageView *imageView, uint32_t *buffer_id) {
    *buffer_id = m_image_views.size();
    CHECK_RESULT(
        vkCreateImageView(m_dev, imageViewCreateInfo, nullptr, imageView),
        "failure to allocate image view");
    m_image_views.emplace(*buffer_id, *imageView);
  }

  void destroy_image_view(VkImageView imageView) {
    vkDestroyImageView(m_dev, imageView, nullptr);
  }

  auto get_shader_module(const std::vector<uint32_t> &code,
                         VkShaderModule *shaderModule, uint32_t *shader_id) {
    *shader_id = m_shader_modules.size();
    VkShaderModuleCreateInfo moduleCreateInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleCreateInfo.pCode = code.data();
    moduleCreateInfo.codeSize = code.size() * sizeof(uint32_t);
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.pNext = nullptr;
    CHECK_RESULT(
        vkCreateShaderModule(m_dev, &moduleCreateInfo, nullptr, shaderModule),
        " failed to create shaderModule");
    m_shader_modules.emplace(*shader_id, *shaderModule);
  }

  void destroy_shader_module(VkShaderModule shaderModule) const {
    vkDestroyShaderModule(m_dev, shaderModule, nullptr);
  }

  void get_pipeline_layout(VkPipelineLayoutCreateInfo *layoutInfo,
                           VkPipelineLayout *layout, uint32_t *layout_id) {
    *layout_id = m_pipeline_layout.size();
    vkCreatePipelineLayout(m_dev, layoutInfo, nullptr, layout);
    m_pipeline_layout.emplace(*layout_id, *layout);
  }

  void destroy_pipeline_layout(VkPipelineLayout pipelineLayout) const {
    vkDestroyPipelineLayout(m_dev, pipelineLayout, nullptr);
  }

  void get_pipeline(VkComputePipelineCreateInfo *stageInfo,
                    VkPipeline *pipeline, uint32_t *pipeline_id) {
    *pipeline_id = m_pipelines.size();
    vkCreateComputePipelines(m_dev, m_pipeline_cache, 1, stageInfo, nullptr,
                             pipeline);
    m_pipelines.emplace(*pipeline_id, *pipeline);
  }

  void destroy_pipeline(VkPipeline pipeline) const {
    vkDestroyPipeline(m_dev, pipeline, nullptr);
  }

  template <uint32_t x, uint32_t y, uint32_t z> friend class Program;
  friend class StorageBuffer;

public:
  Device(const Device &other) {
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

  Device &operator=(const Device &other) {
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

  Device(VkInstance &inst, VkPhysicalDevice &pd) : m_pDev(pd) {
    m_desc_pool_alloc =
        new DescriptorAllocator({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}});
    m_desc_layout_cache = new DescriptorLayoutCache;
    getFeaturesAndProperties(pd);
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount,
                                             queueFamilies.data());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    size_t max_queues = findQueueFamilies(queueFamilies, queueCreateInfos);
    printf("Device has %zi queue families\n", max_queues);

    std::vector<const char *> deviceExtensions;
    if (m_feats.coo_matrix_features.cooperativeMatrix) {
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
    pipelineCacheCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    vkCreatePipelineCache(m_dev, &pipelineCacheCreateInfo, nullptr,
                          &m_pipeline_cache);

    VolkDeviceTable dt;
    volkLoadDeviceTable(&dt, m_dev);

    VmaVulkanFunctions vmaFuncs = {};
    vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vmaFuncs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vmaFuncs.vkGetPhysicalDeviceMemoryProperties =
        vkGetPhysicalDeviceMemoryProperties;
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
    allocatorInfo.preferredLargeHeapBlockSize = static_cast<VkDeviceSize>(1)
                                                << 28;
    allocatorInfo.vulkanApiVersion = volkGetInstanceVersion();
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

    CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator),
                 "Failed to create allocator");
  }

  void destory() {
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

  auto getMemoryProperty(VmaAllocation &alloc) {
    VkMemoryPropertyFlags flags;
    vmaGetAllocationMemoryProperties(m_allocator, alloc, &flags);
    return flags;
  }

  auto flushAllocation(VmaAllocation &alloc, size_t offset, size_t size) const {
    CHECK_RESULT(vmaFlushAllocation(m_allocator, alloc, offset, size),
                 "failed to flush memory allocation");
  }

  auto getDescriptorSet(uint32_t set_number,
                        VkDescriptorSetLayoutCreateInfo *create_info,
                        std::vector<VkWriteDescriptorSet> &writes,
                        VkDescriptorSet &set, VkDescriptorSetLayout &layout) {
    layout = m_desc_layout_cache->create_descriptor_layout(m_dev, set_number,
                                                           create_info);
    bool success = m_desc_pool_alloc->allocate(m_dev, &set, &layout);
    if (!success)
      return false;
    for (VkWriteDescriptorSet &w : writes)
      w.dstSet = set;
    vkUpdateDescriptorSets(m_dev, writes.size(), writes.data(), 0, nullptr);
    return true;
  }



  void getCmdBuffer(VkShaderStageFlagBits shaderFlag,
                    VkCommandBuffer *cmdBuffer) {
    switch (shaderFlag) {
    case VK_SHADER_STAGE_COMPUTE_BIT:
      auto tmp = m_queue_map.equal_range(VK_QUEUE_COMPUTE_BIT);
      size_t q_idx = 0;
      uint32_t c_idx = 0;
      m_queues[tmp.first->second].get_cmd_buffer(cmdBuffer, q_idx, c_idx);
      break;
    }
  }
};

class StorageBuffer {
protected:
  VkBuffer m_buffer = nullptr;
  VkImage m_image = nullptr;
  VkBufferView m_buffer_view = nullptr;
  VkImageView m_image_view = nullptr;

  VmaAllocation m_allocation = nullptr;
  VmaAllocationInfo m_allocation_info = {};

  VkBufferCreateInfo m_buffer_create_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  VkImageCreateInfo m_image_create_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  VkBufferViewCreateInfo m_buffer_view_create_info{
      VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
  VkImageViewCreateInfo m_image_view_create_info{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

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

  enum class BUFFERTYPE { BUFFER, IMAGE, BUFFER_VIEW, IMAGE_VIEW } m_type;

  void setFlags(int flags) {

    if (flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
      m_allocation_create_info.flags |=
          VMA_ALLOCATION_CREATE_MAPPED_BIT |
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    else if (m_is_mapped || flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT ||
             flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ||
             flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
      m_allocation_create_info.flags |=
          VMA_ALLOCATION_CREATE_MAPPED_BIT |
          VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    else
      m_allocation_create_info.flags |=
          VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    m_allocation_create_info.priority = 1.0f;
    m_allocation_create_info.pool = nullptr;
    m_allocation_create_info.requiredFlags = 0;
    m_allocation_create_info.preferredFlags = 0;
    m_allocation_create_info.memoryTypeBits = UINT32_MAX;
  }
  void map() {
    if (m_is_mapped && m_allocation_info.pMappedData != nullptr) {
      m_data = m_allocation_info.pMappedData;
    } else if (m_is_mapped) {
      m_dev->map_buffer(m_allocation, &m_data);
    }
  }

  void lazy_load() {
    if (m_type == BUFFERTYPE::BUFFER && m_buffer == nullptr) {
      m_dev->allocate_buffer(&m_buffer_create_info, &m_allocation_create_info,
                             &m_buffer, &m_allocation, &m_allocation_info,
                             &m_buffer_id);
      map();
    }
    if (m_type == BUFFERTYPE::IMAGE && m_image == nullptr) {
      m_dev->allocate_image(&m_image_create_info, &m_allocation_create_info,
                            &m_image, &m_allocation, &m_allocation_info,
                            &m_buffer_id);
      map();
    }
    if (m_type == BUFFERTYPE::BUFFER_VIEW && m_buffer_view == nullptr)
      m_dev->allocate_buffer_view(&m_buffer_view_create_info, &m_buffer_view,
                                  &m_buffer_id);
    if (m_type == BUFFERTYPE::IMAGE_VIEW && m_image_view == nullptr)
      m_dev->allocate_image_view(&m_image_view_create_info, &m_image_view,
                                 &m_buffer_id);
  }

public:
  StorageBuffer(Device &dev, size_t size, VkBufferUsageFlags flags,
                bool is_mapped = false)
      : m_flags(static_cast<int>(flags)), m_is_mapped(is_mapped),
        m_type(BUFFERTYPE::BUFFER) {
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

  StorageBuffer(Device &dev, VkImageUsageFlags flags, uint32_t mipLevels,
                VkImageType imageType, VkFormat format, VkExtent3D extent,
                uint32_t arrayLayers, VkSampleCountFlagBits samples,
                VkImageTiling tiling)
      : m_flags(static_cast<int>(flags)), m_type(BUFFERTYPE::IMAGE) {
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

  StorageBuffer(Device &dev, StorageBuffer &buffer,
                VkImageViewCreateInfo &imageViewCreateInfo)
      : m_type(BUFFERTYPE::IMAGE_VIEW) {
    m_dev = &dev;
    m_image_view_create_info = imageViewCreateInfo;
    m_image_view_create_info.image = buffer.m_image;
  }

  StorageBuffer(Device &dev, StorageBuffer &buffer,
                VkBufferViewCreateInfo &bufferViewCreateInfo)
      : m_type(BUFFERTYPE::BUFFER_VIEW) {
    m_dev = &dev;
    m_buffer_view_create_info = bufferViewCreateInfo;
    m_buffer_view_create_info.buffer = buffer.m_buffer;
  }

  StorageBuffer &operator=(const StorageBuffer &other) {
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

  StorageBuffer(const StorageBuffer &other) {
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

  auto getMemoryProperties() {
    VkMemoryPropertyFlagBits memFlags;
    return m_dev->getMemoryProperty(m_allocation);
  }

  auto getSize() const { return m_buffer_create_info.size; }

  void *data() { return m_data; }

  auto GetAccessFlags() const {
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

  bool isLive(void) { return m_buffer_id != UINT32_MAX; }

  void setWriteDescriptorSet(VkWriteDescriptorSet &write) {
    lazy_load();
    if (m_buffer != nullptr && m_type == BUFFERTYPE::BUFFER) {
      m_desc_info.desc_buffer_info.buffer = m_buffer;
      m_desc_info.desc_buffer_info.offset = 0;
      m_desc_info.desc_buffer_info.range = m_allocation_info.size;
      write.pBufferInfo = &(m_desc_info.desc_buffer_info);
    } else if (m_image != nullptr && (m_type == BUFFERTYPE::IMAGE ||
                                      m_type == BUFFERTYPE::IMAGE_VIEW)) {
      m_desc_info.desc_image_info.imageLayout =
          m_image_create_info.initialLayout;
      if (m_type == BUFFERTYPE::IMAGE_VIEW)
        m_desc_info.desc_image_info.imageView = m_image_view;
      write.pImageInfo = &(m_desc_info.desc_image_info);
    }
  }
};

template <typename T> class SendBuffer : public StorageBuffer {
public:
  SendBuffer(Device &dev, const std::vector<T> &data,
             const StorageBuffer &outputBuffer)
      : StorageBuffer(dev, data.size() * sizeof T,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true) {
    auto memProps = dev.getMemoryProperty(m_allocation);
    memcpy(dataPtr(), data.data(), data.size() * sizeof T);
    if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      if (getMemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        m_dev->flushAllocation(m_allocation, 0, sizeof(T) * data.size());

      VkBufferMemoryBarrier bufMemBarrier = {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
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

template <typename T> class RecvBuffer : public StorageBuffer {
public:
  RecvBuffer(Device &dev, std::vector<T> &data, StorageBuffer &inputBuffer)
      : StorageBuffer(dev, inputBuffer.getSize(),
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT, true) {
    auto memProps = dev.getMemoryProperty(m_allocation);
    if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      if (getMemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        m_dev->flushAllocation(m_allocation, 0, sizeof(T) * data.size());

      VkBufferMemoryBarrier bufMemBarrier = {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
      bufMemBarrier.srcAccessMask = inputBuffer.GetAccessFlags();
      bufMemBarrier.dstAccessMask = GetAccessFlags();
      bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      bufMemBarrier.buffer = m_buffer;
      bufMemBarrier.offset = 0;
      bufMemBarrier.size = VK_WHOLE_SIZE;
    }

    memcpy_s(data.data(), data.size() * sizeof T, dataPtr(),
             inputBuffer.getSize());
  }
};

template <uint32_t x, uint32_t y, uint32_t z> class Program {
  VkFence *m_fence = nullptr;
  VkSubmitInfo m_submit_info = {};
  VkCommandBuffer m_cmd = nullptr;
  VkPipeline m_pipeline = nullptr;
  VkPipelineLayout m_pipeline_layout = nullptr;

  VkShaderModule m_module;
  SpvReflectShaderModule m_spv_module = {};
  struct set {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkWriteDescriptorSet> writes;
    VkDescriptorSetLayout layout = nullptr;
    VkDescriptorSet descriptorSet;
  };

  std::vector<struct set> m_sets;
  Device *m_dev = nullptr;
  uint32_t m_program_id = UINT32_MAX;
  uint32_t m_pipeline_layout_id = UINT32_MAX;
  uint32_t m_pipeline_id = UINT32_MAX;


public:
  Program(Device &dev, const std::vector<uint32_t> &code,
          std::vector<StorageBuffer> &bufs) {
    m_dev = &dev;
    m_dev->get_shader_module(code, &m_module, &m_program_id);

    SpvReflectResult result = spvReflectCreateShaderModule(
        code.size() * sizeof(uint32_t), code.data(), &m_spv_module);
    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&m_spv_module, &count, NULL);
    std::vector<SpvReflectDescriptorSet *> sets(count);
    m_sets.resize(count);
    result =
        spvReflectEnumerateDescriptorSets(&m_spv_module, &count, sets.data());
    size_t l = 0;
    for (size_t i = 0; i < sets.size(); ++i) {
      const SpvReflectDescriptorSet &refl_set = *(sets[i]);
      auto &set = m_sets[i];
      set.bindings.resize(refl_set.binding_count);
      set.writes.resize(refl_set.binding_count);
      for (size_t j = 0; j < refl_set.binding_count; ++j) {
        auto &binding = set.bindings[j];
        const SpvReflectDescriptorBinding &refl_binding =
            *(refl_set.bindings[j]);
        binding.binding = refl_binding.binding;
        binding.descriptorType =
            static_cast<VkDescriptorType>(refl_binding.descriptor_type);
        binding.descriptorCount = 1;

        for (uint32_t k = 0; k < refl_binding.array.dims_count; ++k)
          binding.descriptorCount *= refl_binding.array.dims[k];
        binding.stageFlags =
            static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage);

        auto &write = set.writes[j];
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.descriptorCount = binding.descriptorCount;
        write.descriptorType = binding.descriptorType;
        write.dstBinding = binding.binding;
        bufs[l++].setWriteDescriptorSet(write);
      }
      VkDescriptorSetLayoutCreateInfo create_info = {
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
      create_info.bindingCount = static_cast<uint32_t>(set.bindings.size());
      create_info.pBindings = set.bindings.data();
      m_dev->getDescriptorSet(i, &create_info, set.writes, set.descriptorSet,
                              set.layout);
    }

    VkPipelineShaderStageCreateInfo stageInfo{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.pNext = nullptr;
    stageInfo.stage =
        static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage);
    stageInfo.module = m_module;
    stageInfo.pName = m_spv_module.entry_point_name;
    m_dev->getCmdBuffer(stageInfo.stage, &m_cmd);
    VkPipelineLayoutCreateInfo layoutInfo{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.pNext = nullptr;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(m_sets.size());
    std::vector<VkDescriptorSetLayout> layouts(m_sets.size());
    for (size_t i = 0; i < m_sets.size(); ++i)
      layouts[i] = m_sets[i].layout;
    layoutInfo.pSetLayouts = layouts.data();
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    m_dev->get_pipeline_layout(&layoutInfo, &m_pipeline_layout, &m_pipeline_layout_id);

    switch (static_cast<VkShaderStageFlagBits>(m_spv_module.shader_stage)) {
    case VK_SHADER_STAGE_COMPUTE_BIT:
      VkComputePipelineCreateInfo pipelineInfo{
          VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
      pipelineInfo.pNext = nullptr;
      pipelineInfo.stage = stageInfo;
      pipelineInfo.layout = m_pipeline_layout;
      m_dev->get_pipeline(&pipelineInfo, &m_pipeline, &m_pipeline_id);

      break;
    };

    if (x != 0 && y != 0 && z != 0) {
      record(x, y, z);
    }
  }

  void record(uint32_t x, uint32_t y, uint32_t z) {
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_cmd, &beginInfo);
    vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    for (auto &set : m_sets) {
      vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                              m_pipeline_layout, 0, 1, &set.descriptorSet, 0,
                              nullptr);
    }
    vkCmdDispatch(m_cmd, x, y, z);
    vkEndCommandBuffer(m_cmd);
  }
};

}; // namespace vkrt