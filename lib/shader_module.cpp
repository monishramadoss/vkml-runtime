#include "shader_module.h"
#include "error_handling.h"

#include <spirv_reflect.h>
#include <vulkan/vulkan.h>
#include <algorithm>

namespace runtime {

    ShaderModule::~ShaderModule()
    {
        cleanup();
    }

    std::shared_ptr<ShaderModule> ShaderModule::create(
        VkDevice device,
        const std::vector<uint32_t>& spirvCode)
    {
        return std::shared_ptr<ShaderModule>(new ShaderModule(device, spirvCode));
    }

    ShaderModule::ShaderModule(VkDevice device, const std::vector<uint32_t>& spirvCode)
        : m_device(device)
        , m_spirvCode(spirvCode)
        , m_module(VK_NULL_HANDLE)
    {
        initialize();
        parseReflectionData();
    }

    void ShaderModule::initialize()
    {
        VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = m_spirvCode.size() * sizeof(uint32_t);
        createInfo.pCode = m_spirvCode.data();

        VkResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, &m_module);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module");
        }
    }

    void ShaderModule::parseReflectionData()
    {
        SpvReflectShaderModule reflectModule;
        SpvReflectResult result = spvReflectCreateShaderModule(m_spirvCode.size() * sizeof(uint32_t),
                                                               m_spirvCode.data(),
                                                               &reflectModule);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to reflect shader module");
        }

        uint32_t numResources = 0;
        result = spvReflectEnumerateDescriptorSets(&reflectModule, &numResources, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            spvReflectDestroyShaderModule(&reflectModule);
            throw std::runtime_error("Failed to enumerate descriptor sets");
        }

        if (numResources == 0) {
            spvReflectDestroyShaderModule(&reflectModule);
            // No resources found, but that's not necessarily an error
            return;
        }

        std::vector<SpvReflectDescriptorSet*> sets(numResources);
        result = spvReflectEnumerateDescriptorSets(&reflectModule, &numResources, sets.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            spvReflectDestroyShaderModule(&reflectModule);
            throw std::runtime_error("Failed to enumerate descriptor sets");
        }

        // Extract all bindings from all sets
        for (uint32_t i = 0; i < numResources; ++i) {
            SpvReflectDescriptorSet* set = sets[i];
            for (uint32_t j = 0; j < set->binding_count; ++j) {
                SpvReflectDescriptorBinding* binding = set->bindings[j];
                ResourceBinding resourceBinding;
                resourceBinding.set = set->set;
                resourceBinding.binding = binding->binding;
                resourceBinding.descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type);
                resourceBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; // Since we're focused on compute
                resourceBinding.descriptorCount = binding->count;
                resourceBinding.name = binding->name ? binding->name : "";
                m_resourceBindings.push_back(resourceBinding);
            }
        }

        // Cleanup reflection data
        spvReflectDestroyShaderModule(&reflectModule);

        // Sort resource bindings by set and binding for easier lookup
        std::sort(m_resourceBindings.begin(), m_resourceBindings.end(),
                  [](const ResourceBinding& a, const ResourceBinding& b) {
                      if (a.set != b.set) {
                          return a.set < b.set;
                      }
                      return a.binding < b.binding;
                  });
                  
        // Log number of bindings found
        if (!m_resourceBindings.empty()) {
            // Success - bindings found and processed
        }
    }

    void ShaderModule::cleanup()
    {
        if (m_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device.getHandle(), m_module, nullptr);
            m_module = VK_NULL_HANDLE;
        }
    }

    const std::vector<ShaderModule::ResourceBinding>& ShaderModule::getResourceBindings() const
    {
        return m_resourceBindings;
    }

    VkShaderModule ShaderModule::getHandle() const
    {
        return m_module;
    }

} // namespace runtime


