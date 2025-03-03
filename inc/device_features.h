#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace runtime {

enum class DeviceType {
    CPU = 0,
    DiscreteGPU = 1,
    IntegratedGPU = 2,
    VirtualGPU = 3,
    Other = 4
};

enum class VendorID {
    AMD = 0,      // 4130
    NVIDIA = 1,   // 4318
    Intel = 2,    // 32902
    ARM = 3,      // 5045
    Qualcomm = 4, // 4367
    Apple = 5,    // 4203
    Other = 6
};

class DeviceFeatures {
public:
    struct Features {
        VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR};
    };

    struct Properties {
        VkPhysicalDeviceProperties2 device_properties_2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        VkPhysicalDeviceSubgroupProperties subgroup_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        VkPhysicalDeviceVulkan11Properties device_vulkan11_properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        VkPhysicalDeviceVulkan12Properties device_vulkan12_properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        VkPhysicalDeviceVulkan13Properties device_vulkan13_properties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    };

    DeviceFeatures(VkPhysicalDevice pd);

    const Features& getFeatures() const { return m_features; }
    const Properties& getProperties() const { return m_properties; }
    
    std::vector<std::string> getDeviceCapabilities() const;
    std::vector<std::string> getSupportedExtensions() const;
    DeviceType getDeviceType() const;
    VendorID getVendorID() const;
    std::vector<uint32_t> getResourceLimits() const;
    
    size_t getMaxAllocationSize() const;
    size_t getSparseAllocationSize() const;
    bool supportsSparseBinding() const;
    bool supportsSparseResidency() const;
    bool supportsSparseResidencyAliased() const;

private:
    void initializeFeatures();
    void initializeProperties();

    VkPhysicalDevice m_physical_device;
    Features m_features;
    Properties m_properties;
};

} // namespace runtime
