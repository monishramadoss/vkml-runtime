#include "device_features.h"

#ifndef VOLK_HH
#define VOLK_HH
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif // VOLK_HH


namespace runtime {
    DeviceFeatures::DeviceFeatures(VkPhysicalDevice& pd)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        m_layers.resize(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, m_layers.data());


        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, nullptr);
        m_extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, m_extensions.data());


        m_features.features2.pNext = &m_features.features11;
        m_features.features11.pNext = &m_features.features12;
        m_features.features12.pNext = &m_features.features13;
        m_features.features13.pNext = &m_features.coo_matrix_features;
        m_features.coo_matrix_features.pNext = nullptr;
        vkGetPhysicalDeviceFeatures2(pd, &m_features.features2);

        m_properties.device_properties_2.pNext = &m_properties.device_vulkan11_properties;
        m_properties.device_vulkan11_properties.pNext = &m_properties.device_vulkan12_properties;
        m_properties.device_vulkan12_properties.pNext = &m_properties.device_vulkan13_properties;
        m_properties.device_vulkan13_properties.pNext = &m_properties.subgroup_properties;
        vkGetPhysicalDeviceProperties2(pd, &m_properties.device_properties_2);
    }

    std::vector<std::string> DeviceFeatures::getDeviceCapabilities() const
    {
        std::vector<std::string> capabilities;
        if (m_features.features2.features.geometryShader)
        {
            capabilities.push_back("Geometry Shader");
        }
        if (m_features.features2.features.tessellationShader)
        {
            capabilities.push_back("Tessellation Shader");
        }
        if (m_features.features2.features.shaderInt64)
        {
            capabilities.push_back("Int64 Shader");
        }
        if (m_features.features2.features.shaderFloat64)
        {
            capabilities.push_back("Float64 Shader");
        }
        if (m_features.features2.features.shaderInt16)
        {
            capabilities.push_back("Int16 Shader");
        }
       
        capabilities.push_back("Int8 Shader");        
        capabilities.push_back("Float16 Shader");
               
        return capabilities;
    }

    std::vector<const char*> DeviceFeatures::getSupportedLayers() const
    {
        std::vector<const char *> layers;
        for (const auto &layer : m_layers)
        {
            layers.push_back(layer.layerName);
        }
        return layers;
    }

    std::vector<const char *> DeviceFeatures::getSupportedExtensions() const
    {
        std::vector<const char *> extensions;
        for (const auto &extension : m_extensions)
        {
            extensions.push_back(extension.extensionName);
        }
        return extensions;
    }

    DeviceType DeviceFeatures::getDeviceType() const noexcept
    {
        if (m_properties.device_properties_2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            return DeviceType::IntegratedGPU;
        }
        else if (m_properties.device_properties_2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            return DeviceType::DiscreteGPU;
        }
        else if (m_properties.device_properties_2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        {
            return DeviceType::VirtualGPU;
        }
        else if (m_properties.device_properties_2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
        {
            return DeviceType::CPU;
        }
        else
        {
            return DeviceType::Other;
        }
    }

    VendorID DeviceFeatures::getVendorID() const noexcept
    {
        if (m_properties.device_properties_2.properties.vendorID == 0x1002)
        {
            return VendorID::AMD;
        }
        else if (m_properties.device_properties_2.properties.vendorID == 0x10DE)
        {
            return VendorID::NVIDIA;
        }
        else if (m_properties.device_properties_2.properties.vendorID == 0x8086)
        {
            return VendorID::Intel;
        }
        else if (m_properties.device_properties_2.properties.vendorID == 0x13B5)
        {
            return VendorID::ARM;
        }
        else if (m_properties.device_properties_2.properties.vendorID == 0x5143)
        {
            return VendorID::Qualcomm;
        }
        else if (m_properties.device_properties_2.properties.vendorID == 0x106B)
        {
            return VendorID::Apple;
        }
        else
        {
            return VendorID::Other;
        }
    }

    std::vector<uint32_t> DeviceFeatures::getResourceLimits() const
    {
        std::vector<uint32_t> limits;
        limits.push_back(m_properties.device_properties_2.properties.limits.maxImageDimension1D);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxImageDimension2D);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxImageDimension3D);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxImageDimensionCube);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxImageArrayLayers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTexelBufferElements);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxUniformBufferRange);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxStorageBufferRange);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPushConstantsSize);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxMemoryAllocationCount);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxSamplerAllocationCount);
        limits.push_back(m_properties.device_properties_2.properties.limits.bufferImageGranularity);
        limits.push_back(m_properties.device_properties_2.properties.limits.sparseAddressSpaceSize);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxBoundDescriptorSets);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageDescriptorSamplers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageDescriptorUniformBuffers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageDescriptorStorageBuffers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageDescriptorSampledImages);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageDescriptorStorageImages);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageDescriptorInputAttachments);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxPerStageResources);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetSamplers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetUniformBuffers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetUniformBuffersDynamic);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetStorageBuffers);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetStorageBuffersDynamic);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetSampledImages);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetStorageImages);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxDescriptorSetInputAttachments);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxVertexInputAttributes);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxVertexInputBindings);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxVertexInputAttributeOffset);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxVertexInputBindingStride);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxVertexOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationGenerationLevel);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationPatchSize);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationControlPerVertexInputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationControlPerVertexOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationControlPerPatchOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationControlTotalOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationEvaluationInputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxTessellationEvaluationOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxGeometryShaderInvocations);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxGeometryInputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxGeometryOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxGeometryOutputVertices);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxGeometryTotalOutputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxFragmentInputComponents);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxFragmentOutputAttachments);
        limits.push_back(m_properties.device_properties_2.properties.limits.maxFragmentDualSrcAttachments);

        return limits;
    }

    size_t DeviceFeatures::getMaxAllocationSize() const noexcept
    {
        return m_properties.device_properties_2.properties.limits.maxStorageBufferRange;
    }

    size_t DeviceFeatures::getSparseAllocationSize() const noexcept
    {
        return m_properties.device_properties_2.properties.limits.sparseAddressSpaceSize;
    }

    bool DeviceFeatures::supportsSparseBinding() const noexcept
    {
        return m_features.coo_matrix_features.cooperativeMatrix;
    }

    bool DeviceFeatures::supportsSparseResidency() const noexcept
    {
        return false;
    }

    bool DeviceFeatures::supportsSparseResidencyAliased() const noexcept
    {
        return false;
    }

    std::string DeviceFeatures::getDeviceName() const
    {
        return std::string();
    }


} // namespace runtime


