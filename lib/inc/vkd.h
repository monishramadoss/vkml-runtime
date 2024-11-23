#pragma once
#include "vka.h"
#include "vke.h"
#include <spirv_reflect.h>
#include <thread>
#include <vk_mem_alloc.h>

namespace vkrt
{

class Device
{

    VkDevice m_dev;
    VkPipelineCache m_pipeline_cache;
    VkPhysicalDevice m_pDev;

    VmaAllocator m_allocator;
    std::vector<std::shared_ptr<QueueDispatcher>> m_queues;
    std::multimap<VkQueueFlags, uint32_t> m_queue_map;

    ExecutionGraph execGraph;

    // TODO make a descriptor pool per thread; and have thread access each one independentaly

    DescriptorAllocator *m_desc_pool_alloc;
    DescriptorLayoutCache *m_desc_layout_cache;

    void setupQueues(std::vector<VkQueueFamilyProperties> &queueFamilies)
    {
        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            m_queues.emplace_back(std::make_shared<QueueDispatcher>(i, m_dev, queueFamilies[i]));
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

    uint32_t supported_data_types() const
    {
        m_feats.features2.features.shaderFloat64;
        m_feats.features2.features.shaderInt16;
        m_feats.features2.features.shaderInt64;
        m_feats.features12.shaderFloat16;
        m_feats.features12.shaderInt8;
        return 0;
    }

    void map_buffer(VmaAllocation alloc, void **data) const
    {
        vmaMapMemory(m_allocator, alloc, data);
    }

    void unmap_buffer(VmaAllocation alloc) const
    {
        vmaUnmapMemory(m_allocator, alloc);
    }

    auto copyMemoryToBuffer(VmaAllocation dst, const void *src, size_t size, size_t dst_offset = 0) const
    {
        return vmaCopyMemoryToAllocation(m_allocator, src, dst, dst_offset, size);
    }

    auto copyBufferToMemory(VmaAllocation src, void *dst, size_t size, size_t src_offset = 0) const
    {
        return vmaCopyAllocationToMemory(m_allocator, src, src_offset, dst, size);
    }

    void allocate_buffer(VkBufferCreateInfo *bufferCreateInfo, VmaAllocationCreateInfo *vmaAllocCreateInfo,
                         VkBuffer *buffer, VmaAllocation *allocation, VmaAllocationInfo *allocInfo, uint32_t *buffer_id)
    {
        uint32_t memTypeIdx = 0;
        VkResult res{};

        if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
            VK_SUCCESS)
        {
            vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
            res = vmaCreateBuffer(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, buffer, allocation, allocInfo);
        }
        else
        {
            printf("Failed to find memory type index\n");
            return;
        }
    }

    std::shared_ptr<vkrt_buffer> construct(const VkBufferCreateInfo *bufferCreateInfo,
                                           VmaAllocationCreateInfo *vmaAllocCreateInfo);

    std::shared_ptr<vkrt_compute_program> construct(
        const std::vector<uint32_t> &code);

    void update(uint32_t set_id, std::shared_ptr<vkrt_compute_program> program,
                std::vector<std::shared_ptr<vkrt_buffer>> &buffers) const
    {
        for (uint32_t j = 0; j < program->n_set_bindings[set_id]; ++j)
        {
            VkDescriptorBufferInfo desc_buffer_info = {buffers[j]->buffer, 0, buffers[j]->allocation_info.size};
            program->writes[set_id][j].pBufferInfo = &desc_buffer_info;
        }

        vkUpdateDescriptorSets(m_dev, program->n_set_bindings[set_id], program->writes[set_id], 0, nullptr);
    }

    size_t getMaxAllocationSize() const
    {
        return std::max<uint64_t>(0, // m_props.device_vulkan13_properties.maxBufferSize,
                                  m_props.device_vulkan11_properties.maxMemoryAllocationSize);
    }

    size_t getSparseAllocationSize() const
    {
        return std::max<uint64_t>(m_props.device_properties_2.properties.limits.maxStorageBufferRange,
                                  m_props.device_properties_2.properties.limits.sparseAddressSpaceSize);
    }

    std::vector<uint32_t> getSparseQueueFamily() const
    {
        std::vector<uint32_t> queues;
        for (auto &q : m_queues)
        {
            if (q->isSparseBindingFamily())
            {
                queues.push_back(q->get_idx());
            }
        }
        return queues;
    }


    void submit(struct ComputePacket &packet)
    {
        auto qit = m_queue_map.equal_range(VK_QUEUE_COMPUTE_BIT);
        for (auto &qidx = qit.first; qidx != qit.second; ++qidx)
        {
            std::shared_ptr<QueueDispatcher> &q = m_queues.at(qidx->second);
            if (!q->isBusy())
            {
                q->submit(packet);
                break;
            }
        }
        throw std::runtime_error("No compute queue available");
    }

    void constructEvent(VkEventCreateInfo *info, VkEvent *event) const
    {
        vkCreateEvent(m_dev, info, nullptr, event);
    }

    void destoryEvent(VkEvent event) const
    {
        vkDestroyEvent(m_dev, event, nullptr);
    }

    friend class ComputeProgram;
    friend class buffer;

  public:
    Device(const Device &other)
    {
        m_dev = other.m_dev;
        m_pipeline_cache = other.m_pipeline_cache;
        m_pDev = other.m_pDev;
        m_allocator = other.m_allocator;
        m_queues = other.m_queues;
        execGraph = other.execGraph;
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
        execGraph = other.execGraph;
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
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
        deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
        VkPhysicalDeviceFeatures2 features2 = m_feats.features2;
        VkPhysicalDeviceVulkan11Features features11 = m_feats.features11;
        VkPhysicalDeviceVulkan12Features features12 = m_feats.features12;
        VkPhysicalDeviceVulkan13Features features13 = m_feats.features13;
        features13.pNext = nullptr;
        features2.pNext = &features11;
        features11.pNext = &features12;
        features12.pNext = &features13;
        deviceInfo.pNext = &features2;

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
        allocatorInfo.preferredLargeHeapBlockSize = getMaxAllocationSize();
        allocatorInfo.vulkanApiVersion = volkGetInstanceVersion();
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
                              VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                              VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;

        CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator), "Failed to create allocator");
    }

    void destory()
    {
        /*
            buffers
            compute programs
        */
        for (auto i = 0; i < execGraph.size(); ++i)
        {
            auto node = execGraph.getNode(i);

            if (node->sType == vkrtType::BUFFER)
            {
                auto buffer = std::static_pointer_cast<vkrt_buffer>(node);
                vkDestroyBuffer(m_dev, buffer->buffer, nullptr);
                vmaFreeMemory(m_allocator, buffer->allocation);
            }
            else if (node->sType == vkrtType::COMPUTE_PROGRAM)
            {
                auto program = std::static_pointer_cast<vkrt_compute_program>(node);
                for (auto i = 0; i < program->n_sets; ++i)
                {
                    delete[] program->bindings[i];
                    delete[] program->writes[i];                    
                    vkDestroyDescriptorSetLayout(m_dev, program->layouts[i], nullptr);
                }
                vkDestroyShaderModule(m_dev, program->module, nullptr);
                vkDestroyPipelineLayout(m_dev, program->pipeline_layout, nullptr);
                vkDestroyPipeline(m_dev, program->pipeline, nullptr);
                delete[] program->writes;
                delete[] program->bindings;
                delete[] program->layouts;
                delete[] program->n_set_bindings;
                delete program->sets;
            }
        }
        for (auto &q : m_queues)
            q->destroy(m_dev);
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
};
std::shared_ptr<vkrt_buffer> Device::construct(const VkBufferCreateInfo *bufferCreateInfo,
                                               VmaAllocationCreateInfo *vmaAllocCreateInfo)
{
    std::shared_ptr<vkrt_buffer> buffer;
    buffer->sType = vkrtType::BUFFER;
    uint32_t memTypeIdx = 0;
    VkResult res{};

    if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
        VK_SUCCESS)
    {
        vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
        res = vmaCreateBuffer(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &buffer->buffer, &buffer->allocation,
                              &buffer->allocation_info);
    }
    if (buffer->buffer != nullptr && res == VK_SUCCESS)
        execGraph.addNode(buffer);

    return buffer;
}

std::shared_ptr<vkrt_compute_program> Device::construct(
        const std::vector<uint32_t> &code)
{

    VkPipelineShaderStageCreateInfo stageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    VkPipelineLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VkComputePipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    VkShaderModuleCreateInfo moduleInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

    SpvReflectShaderModule spv_module = {};
    std::shared_ptr<vkrt_compute_program> program = std::make_shared<vkrt_compute_program>();
    program->sType = vkrtType::COMPUTE_PROGRAM;
    SpvReflectResult result = spvReflectCreateShaderModule(code.size() * sizeof(uint32_t), code.data(), &spv_module);
    result = spvReflectEnumerateDescriptorSets(&spv_module, &program->n_sets, NULL);
    program->n_set_bindings = new uint32_t[program->n_sets];
    program->layouts = new VkDescriptorSetLayout[program->n_sets];
    program->sets = new VkDescriptorSet[program->n_sets];
    program->writes = new VkWriteDescriptorSet *[program->n_sets];
    program->bindings = new VkDescriptorSetLayoutBinding *[program->n_sets];
    std::vector<SpvReflectDescriptorSet *> sets(program->n_sets);
    result = spvReflectEnumerateDescriptorSets(&spv_module, &program->n_sets, sets.data());
    for (size_t i = 0; i < sets.size(); ++i)
    {
        const SpvReflectDescriptorSet &refl_set = *(sets[i]);
        program->bindings[i] = new VkDescriptorSetLayoutBinding[refl_set.binding_count];
        program->writes[i] = new VkWriteDescriptorSet[refl_set.binding_count];
        for (size_t j = 0; j < refl_set.binding_count; ++j)
        {
            const SpvReflectDescriptorBinding &refl_binding = *(refl_set.bindings[j]);
            program->n_set_bindings[i] = refl_set.binding_count;
            program->bindings[i][j].binding = refl_binding.binding;
            program->bindings[i][j].descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
            program->bindings[i][j].stageFlags = static_cast<VkShaderStageFlagBits>(spv_module.shader_stage);
            program->bindings[i][j].descriptorCount = 1;
            for (uint32_t k = 0; k < refl_binding.array.dims_count; ++k)
                program->bindings[i][j].descriptorCount *= refl_binding.array.dims[k];
            program->writes[i][j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            program->writes[i][j].descriptorCount = program->bindings[i][j].descriptorCount;
            program->writes[i][j].descriptorType = program->bindings[i][j].descriptorType;
            program->writes[i][j].dstBinding = program->bindings[i][j].binding;

        }

        VkDescriptorSetLayoutCreateInfo create_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        create_info.bindingCount = static_cast<uint32_t>(refl_set.binding_count);
        create_info.pBindings = program->bindings[i];
        program->layouts[i] = m_desc_layout_cache->create_descriptor_layout(m_dev, i, &create_info);
        bool success = m_desc_pool_alloc->allocate(m_dev, &program->sets[i], &program->layouts[i]);
        for (size_t j = 0; j < refl_set.binding_count; ++j)
            program->writes[i][j].dstSet = program->sets[i];
    }
   
    moduleInfo.pCode = code.data();
    moduleInfo.codeSize = code.size() * sizeof(uint32_t);
    moduleInfo.flags = 0;
    moduleInfo.pNext = nullptr;
    CHECK_RESULT(vkCreateShaderModule(m_dev, &moduleInfo, nullptr, &program->module),
                 " failed to create shaderModule");

    stageInfo.pNext = nullptr;
    stageInfo.stage = static_cast<VkShaderStageFlagBits>(spv_module.shader_stage);
    stageInfo.module = program->module;
    stageInfo.pName = spv_module.entry_point_name;

    layoutInfo.pNext = nullptr;
    layoutInfo.setLayoutCount = program->n_sets;
    layoutInfo.pSetLayouts = program->layouts;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;
    vkCreatePipelineLayout(m_dev, &layoutInfo, nullptr, &program->pipeline_layout);

    pipelineInfo.pNext = nullptr;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = program->pipeline_layout;
    vkCreateComputePipelines(m_dev, m_pipeline_cache, 1, &pipelineInfo, nullptr, &program->pipeline);

    execGraph.addNode(program);
    return program;
}





} // namespace vkrt

/*
    auto &q = getSparseQueueFamily();
    if (bufferCreateInfo->size >= getSparseAllocationSize())
    {
        return;
    }

    if (m_feats.features2.features.sparseBinding)
    {
        bufferCreateInfo->sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferCreateInfo->flags = VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
        bufferCreateInfo->pQueueFamilyIndices = q.data();
        bufferCreateInfo->queueFamilyIndexCount = q.size();
    }
    if (m_feats.features2.features.sparseResidencyAliased)
    {
        bufferCreateInfo->flags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
        vmaAllocCreateInfo->flags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
    }
    if (m_feats.features2.features.sparseResidencyBuffer)
    {
        bufferCreateInfo->flags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
    }

    res = vkCreateBuffer(m_dev, bufferCreateInfo, nullptr, buffer);
    CHECK_RESULT(res, "failed to create buffer");

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_dev, *buffer, &mem_reqs);

    vmaAllocCreateInfo->requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vmaAllocCreateInfo->preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vmaAllocCreateInfo->usage = VMA_MEMORY_USAGE_UNKNOWN;
    VkSparseMemoryBind binds[MAX_CHUNKS] = {};
    if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) ==
        VK_SUCCESS)
    {
        auto total_size = bufferCreateInfo->size;
        size_t num_blocks = std::ceil<size_t>(bufferCreateInfo->size / getMaxAllocationSize());
        vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;

        mem_reqs.size = getMaxAllocationSize();
        vmaAllocateMemoryPages(m_allocator, &mem_reqs, vmaAllocCreateInfo, num_blocks, allocation.data(),
                                allocInfo.data());
        size_t bindCount = 0;
        size_t offset = 0;
        for (auto bindCount = 0; bindCount < allocInfo.size(); ++bindCount)
        {
            VkSparseMemoryBind *bind = &binds[bindCount];
            bind->memory = allocInfo[bindCount].deviceMemory;
            bind->memoryOffset = offset;
            bind->resourceOffset = allocInfo[bindCount].offset;
            bind->size = std::min<uint64_t>(allocInfo[bindCount].size, total_size);
            total_size -= bind->size;
            offset += bind->size;
        }
    }
    else
    {
        printf("Failed to find memory type index\n");
    }




*/