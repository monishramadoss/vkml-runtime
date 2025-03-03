#include "shader_module.h"

#include "device.h"
#include "buffer.h"
#include "dispatch_builder.h"
#include "compute_fence.h"

#include <spirv_reflect.h>
#include <vulkan/vulkan.h>

namespace runtime {

    ShaderModule::~ShaderModule()
    {
        cleanup();
    }

    std::shared_ptr<ShaderModule> ShaderModule::create(
        Device& device,
        const std::vector<uint32_t>& spirvCode)
    {
        return std::shared_ptr<ShaderModule>(new ShaderModule(device, spirvCode));
    }

    ShaderModule::ShaderModule(Device& device, const std::vector<uint32_t>& spirvCode)
        : m_device(device)
        , m_spirvCode(spirvCode)
    {
        initialize();
        parseReflectionData();
    }

    void ShaderModule::initialize()
    {
        VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = m_spirvCode.size() * sizeof(uint32_t);
        createInfo.pCode = m_spirvCode.data();

        VK_CHECK(vkCreateShaderModule(m_device.getHandle(), &createInfo, nullptr, &m_module));
    }

    void ShaderModule::cleanup()
    {
        vkDestroyShaderModule(m_device.getHandle(), m_module, nullptr);
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
            throw std::runtime_error("Failed to enumerate descriptor sets");
        }

        std::vector<SpvReflectDescriptorSet*> sets(numResources);
        result = spvReflectEnumerateDescriptorSets(&reflectModule, &numResources, sets.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to enumerate descriptor sets");
        }

        for (uint32_t i = 0; i < numResources; ++i) {
            SpvReflectDescriptorSet* set = sets[i];
            for (uint32_t j = 0; j < set->binding_count; ++j) {
                SpvReflectDescriptorBinding* binding = set->bindings[j];
                ResourceBinding resourceBinding;
                resourceBinding.set = set->set;
                resourceBinding.binding = binding->binding;
                resourceBinding.descriptorType = binding->descriptor_type;
                resourceBinding.stageFlags = binding->accessed_stages;
                resourceBinding.descriptorCount = binding->count;
                m_resourceBindings.push_back(resourceBinding);
            }
        }

        spvReflectDestroyShaderModule(&reflectModule);

        if (m_resourceBindings.empty()) {
            throw std::runtime_error("No resources found in shader module");
        }

        // Sort resource bindings by set and binding

        std::sort(m_resourceBindings.begin(), m_resourceBindings.end(),
                  [](const ResourceBinding& a, const ResourceBinding& b) {
                      if (a.set != b.set) {
                          return a.set < b.set;
                      }
                      return a.binding < b.binding;
                  });

        // Validate resource bindings

        for (size_t i = 0; i < m_resourceBindings.size(); ++i) {
            const auto& binding = m_resourceBindings[i];
            if (binding.set != i) {
                throw std::runtime_error("Invalid resource binding set");
            }
            if (binding.binding != i) {
                throw std::runtime_error("Invalid resource binding binding");
            }
        }

        // Check for duplicate resource bindings

        for (size_t i = 1; i < m_resourceBindings.size(); ++i) {
            const auto& prev = m_resourceBindings[i - 1];
            const auto& curr = m_resourceBindings[i];
            if (prev.set == curr.set && prev.binding == curr.binding) {
                throw std::runtime_error("Duplicate resource binding");
            }
        }

        // Check for missing resource bindings

        for (size_t i = 0; i < m_resourceBindings.size(); ++i) {
            const auto& binding = m_resourceBindings[i];
            if (binding.set != i || binding.binding != i) {
                throw std::runtime_error("Missing resource binding");
            }
        }

        // Check for invalid descriptor types

        for (const auto& binding : m_resourceBindings) {
            if (binding.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
                binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                throw std::runtime_error("Invalid descriptor type");
            }
        }

        // Check for invalid stage flags
        
        for (const auto& binding : m_resourceBindings) {
            if (binding.stageFlags != SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT) {
                throw std::runtime_error("Invalid stage flags");
            }
        }

        // Check for invalid descriptor counts
        for (const auto& binding : m_resourceBindings) {
            if (binding.descriptorCount <= 0) {
                throw std::runtime_error("Invalid descriptor count");
            }
        }
        
        // Log number of resources found
        if (m_resourceBindings.size() > 0) {
            // We've already validated the resources, so now we're good to go
            return;
        }
    }

    void ShaderModule::cleanup()
    {
        vkDestroyShaderModule(m_device.getHandle(), m_module, nullptr);
    }


} // namespace runtime


