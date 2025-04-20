#include "program.h"

#include "error_handling.h"
#include "device.h"
#include "storage.h"
#include "queue.h"

namespace runtime
{

    Program::Program(VkDevice device, VkPipelineCache pipeline_cache,
                std::shared_ptr<DescriptorLayoutCache> &descCache,
                std::shared_ptr<DescriptorAllocator> &descAllocator,                 
                 const std::vector<uint32_t> &shader_code,
                uint32_t dim_x, uint32_t dim_y, uint32_t dim_z)
    : m_device(device), m_module(VK_NULL_HANDLE), m_pipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE),
      dims{dim_x, dim_y, dim_z}
    {       
        initialize(device, pipeline_cache, descCache, descAllocator, shader_code);
    }

    std::shared_ptr<Program> Program::create(VkDevice device, VkPipelineCache pipeline_cache,
                                             std::shared_ptr<DescriptorLayoutCache> &descCache,
                                             std::shared_ptr<DescriptorAllocator> &descAllocator,
                                             const std::vector<uint32_t> &shader_code, uint32_t dim_x, uint32_t dim_y,
                                             uint32_t dim_z)
    {
        return std::make_shared<Program>(device, pipeline_cache, descCache, descAllocator, shader_code, dim_x, dim_y, dim_z);
    }

    Program::~Program()
    {
        cleanup();
    }

    void Program::Arg(std::shared_ptr<Buffer> &buffer, size_t binding_idx, size_t set_idx)
    {
        check_condition(set_idx < writes.size(), "set index out of range");
        check_condition(binding_idx < writes[set_idx].size(), "binding index out of range");
        writes[set_idx][binding_idx].pBufferInfo = buffer->getBufferInfo();
        vkUpdateDescriptorSets(m_device, 1, &writes[set_idx][binding_idx], 0, nullptr);
    }

    void Program::setup(std::shared_ptr<CommandPoolManager> cmd_pool)
    {
        if (!m_cmdPoolManager)
            m_cmdPoolManager = cmd_pool;
        m_cmdPoolManager->submitCompute(m_pipeline, m_pipelineLayout, sets.size(), sets.data(),
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                dims[0], dims[1], dims[2]);
        
    }

    void Program::initialize(VkDevice device, VkPipelineCache pipeline_cache, std::shared_ptr<DescriptorLayoutCache> &descCache,
                             std::shared_ptr<DescriptorAllocator> &descAllocator,
                             const std::vector<uint32_t> &shader_code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shader_code.size() * sizeof(uint32_t);
        createInfo.pCode = shader_code.data();
        vkCreateShaderModule(device, &createInfo, nullptr, &m_module);
        SpvReflectShaderModule ref_module = {};
        SpvReflectResult result = spvReflectCreateShaderModule(createInfo.codeSize, createInfo.pCode, &ref_module);
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to create shader module");

        uint32_t count;
        check_condition(spvReflectEnumerateDescriptorSets(&ref_module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS,
                        "failed to enumerate descriptors");
        std::vector<SpvReflectDescriptorSet *> reflsets(count);
        std::vector<VkDescriptorSetLayout> layouts(count);
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> bindings(count);
        sets.resize(count);

        writes.resize(count);

        check_condition(spvReflectEnumerateDescriptorSets(&ref_module, &count, reflsets.data()) ==
                            SPV_REFLECT_RESULT_SUCCESS,
                        "failed to enumerate descriptors");

        for (size_t i = 0; i < reflsets.size(); ++i)
        {
            const auto &refl_set = *(reflsets[i]);
            bindings[i].resize(refl_set.binding_count);
            writes[i].resize(refl_set.binding_count);
            for (size_t j = 0; j < refl_set.binding_count; ++j)
            {
                const auto &refl_binding = *(refl_set.bindings[j]);
                bindings[i][j].binding = refl_binding.binding;
                bindings[i][j].descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
                bindings[i][j].descriptorCount = 1;
                for (uint32_t k = 0; k < refl_binding.array.dims_count; ++k)
                    bindings[i][j].descriptorCount *= refl_binding.array.dims[k];
                bindings[i][j].stageFlags = static_cast<VkShaderStageFlagBits>(ref_module.shader_stage);
                writes[i][j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i][j].pNext = nullptr;
                writes[i][j].dstSet = VK_NULL_HANDLE;
                writes[i][j].dstBinding = refl_binding.binding;
                writes[i][j].dstArrayElement = 0;
                writes[i][j].descriptorCount = bindings[i][j].descriptorCount;
                writes[i][j].descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);
                writes[i][j].pImageInfo = nullptr;
                writes[i][j].pBufferInfo = nullptr;
                writes[i][j].pTexelBufferView = nullptr;
            }

            VkDescriptorSetLayoutCreateInfo descCreateInfo = {};
            descCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descCreateInfo.bindingCount = static_cast<uint32_t>(bindings[i].size());
            descCreateInfo.pBindings = bindings[i].data();
            descCreateInfo.flags = 0;
            descCreateInfo.pNext = nullptr;

            layouts[i] = descCache->getDescriptorSetLayout(i, &descCreateInfo);
        }
        check_condition(descAllocator->allocate(sets.size(), sets.data(), layouts.data()),
                        "failed to allocate descriptorPool");
        for (size_t i = 0; i < reflsets.size(); ++i)
        {
            for (size_t j = 0; j < reflsets[i]->binding_count; ++j)
            {
                writes[i][j].dstSet = sets[i];
            }
        }

        check_condition(spvReflectEnumerateDescriptorBindings(&ref_module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS,
                        "failed to enumerate bindings");
        std::vector<SpvReflectDescriptorBinding *> desc_bindings(count);
        check_condition(spvReflectEnumerateDescriptorBindings(&ref_module, &count, desc_bindings.data()) ==
                            SPV_REFLECT_RESULT_SUCCESS,
                        "failed to enumerate bindings");

        check_condition(spvReflectEnumerateInterfaceVariables(&ref_module, &count, NULL) == SPV_REFLECT_RESULT_SUCCESS,
                        "failed to enumerate interface variables");
        std::vector<SpvReflectInterfaceVariable *> interface_variables(count);
        check_condition(spvReflectEnumerateInterfaceVariables(&ref_module, &count, interface_variables.data()) ==
                            SPV_REFLECT_RESULT_SUCCESS,
                        "failed to enumerate interface variables");

        result = spvReflectEnumerateInputVariables(&ref_module, &count, NULL);
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "failed to enumerate input variables");
        std::vector<SpvReflectInterfaceVariable *> input_variables(count);
        result = spvReflectEnumerateInputVariables(&ref_module, &count, input_variables.data());
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "failed to enumerate input variables");

        result = spvReflectEnumerateOutputVariables(&ref_module, &count, NULL);
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "failed to enumerate output variables");
        std::vector<SpvReflectInterfaceVariable *> output_variables(count);
        result = spvReflectEnumerateOutputVariables(&ref_module, &count, output_variables.data());
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "failed to enumerate output variables");

        result = spvReflectEnumeratePushConstantBlocks(&ref_module, &count, NULL);
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "failed to enumerate push constants");
        std::vector<SpvReflectBlockVariable *> push_constant(count);
        result = spvReflectEnumeratePushConstantBlocks(&ref_module, &count, push_constant.data());
        check_condition(result == SPV_REFLECT_RESULT_SUCCESS, "failed to enumerate push constants");

        std::string entryName(ref_module.entry_point_name);
        VkPipelineShaderStageCreateInfo stageInfo = {};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = static_cast<VkShaderStageFlagBits>(ref_module.shader_stage);
        stageInfo.pNext = nullptr;
        stageInfo.module = m_module;
        stageInfo.pName = entryName.c_str();

        VkSpecializationInfo specializationInfo = {};
        specializationInfo.mapEntryCount = 0;
        specializationInfo.pMapEntries = nullptr;
        specializationInfo.dataSize = 0;
        specializationInfo.pData = nullptr;
        stageInfo.pSpecializationInfo = &specializationInfo;

        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = nullptr;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        layoutInfo.pSetLayouts = layouts.data();
        layoutInfo.pushConstantRangeCount = 0;
        layoutInfo.pPushConstantRanges = nullptr;

        check_result(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout),
                     "failed create pipelinelayout");

        VkComputePipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.stage = stageInfo;

        check_result(vkCreateComputePipelines(m_device, pipeline_cache, 1, &pipelineInfo, nullptr, &m_pipeline),
                     "failed to create compute pipeline");
        spvReflectDestroyShaderModule(&ref_module);


    }

    void Program::cleanup()
    {
        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }
        if (m_pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }
        if (m_module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_module, nullptr);
            m_module = VK_NULL_HANDLE;
        }
    }
    



    } // namespace runtime
