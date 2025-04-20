#ifndef DEVICE_FEATURES_H
#define DEVICE_FEATURES_H

#include <vector>
#include <string>
#include <unordered_map>

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

#include <string>
namespace runtime {

/**
 * @brief Enumeration of GPU device types
 */
enum class DeviceType {
    CPU = 0,
    DiscreteGPU = 1,
    IntegratedGPU = 2,
    VirtualGPU = 3,
    Other = 4
};

/**
 * @brief GPU vendor identification
 */
enum class VendorID {
    AMD = 0,      // 0x1002 (4098)
    NVIDIA = 1,   // 0x10DE (4318)
    Intel = 2,    // 0x8086 (32902)
    ARM = 3,      // 0x13B5 (5045)
    Qualcomm = 4, // 0x5143 (20803)
    Apple = 5,    // 0x106B (4203)
    Other = 6
};

/**
 * @brief Detection and management of device capabilities
 * 
 * Provides comprehensive access to device features, properties,
 * and capabilities for optimal workload scheduling.
 */
class DeviceFeatures {
public:
   
    struct Features {
        VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR};
    };

    struct Properties {
        VkPhysicalDeviceProperties2 device_properties_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        VkPhysicalDeviceSubgroupProperties subgroup_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        VkPhysicalDeviceVulkan11Properties device_vulkan11_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        VkPhysicalDeviceVulkan12Properties device_vulkan12_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        VkPhysicalDeviceVulkan13Properties device_vulkan13_properties = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    };

  
    DeviceFeatures(VkPhysicalDevice& physical_device);
    
    VkPhysicalDeviceFeatures2 *getFeatures2() noexcept
    {
        return &m_features.features2;
    }

    VkPhysicalDeviceVulkan11Features *getVulkan11Features() noexcept
    {
        return &m_features.features11;
    }
    VkPhysicalDeviceVulkan12Features *getVulkan12Features() noexcept
    {
        return &m_features.features12;
    }
    VkPhysicalDeviceVulkan13Features *getVulkan13Features() noexcept
    {
        return &m_features.features13;
    }

    [[nodiscard]] const Features& getFeatures() const noexcept { return m_features; }
    [[nodiscard]] const Properties& getProperties() const noexcept { return m_properties; }
    [[nodiscard]] std::vector<std::string> getDeviceCapabilities() const;
    [[nodiscard]] std::vector<const char *> getSupportedLayers() const;
    [[nodiscard]] std::vector<const char*> getSupportedExtensions() const;
    [[nodiscard]] DeviceType getDeviceType() const noexcept;
    [[nodiscard]] VendorID getVendorID() const noexcept;
    [[nodiscard]] std::vector<uint32_t> getResourceLimits() const;
    [[nodiscard]] size_t getMaxAllocationSize() const noexcept;
    [[nodiscard]] size_t getSparseAllocationSize() const noexcept;
    [[nodiscard]] bool supportsSparseBinding() const noexcept;
    [[nodiscard]] bool supportsSparseResidency() const noexcept;
    [[nodiscard]] bool supportsSparseResidencyAliased() const noexcept;
    [[nodiscard]] std::string getDeviceName() const;

private:
    Features m_features;
    Properties m_properties;
    std::vector<VkExtensionProperties> m_extensions;   
    std::vector<VkLayerProperties> m_layers;
    // Cache common device info
    static const std::unordered_map<uint32_t, VendorID> s_vendor_map;
};

} // namespace runtime

#endif // DEVICE_FEATURES_H