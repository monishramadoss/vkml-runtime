#ifndef DEVICE_FEATURES_H
#define DEVICE_FEATURES_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>

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
    /**
     * @brief Structure containing all queried device features
     */
    struct Features {
        VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features features11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR};
    };

    /**
     * @brief Structure containing all queried device properties
     */
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

    /**
     * @brief Initialize device features discovery
     * @param physical_device The Vulkan physical device to query
     */
    explicit DeviceFeatures(VkPhysicalDevice physical_device);

    /**
     * @brief Get all queried device features
     * @return Const reference to device features
     */
    [[nodiscard]] const Features& getFeatures() const noexcept { return m_features; }
    
    /**
     * @brief Get all queried device properties
     * @return Const reference to device properties
     */
    [[nodiscard]] const Properties& getProperties() const noexcept { return m_properties; }
    
    /**
     * @brief Get list of device capabilities as strings
     * @return Vector of capability strings
     */
    [[nodiscard]] std::vector<std::string> getDeviceCapabilities() const;
    
    /**
     * @brief Get supported Vulkan extensions
     * @return Vector of extension names
     */
    [[nodiscard]] std::vector<std::string> getSupportedExtensions() const;
    
    /**
     * @brief Get device type (discrete, integrated, etc)
     * @return DeviceType enumeration value
     */
    [[nodiscard]] DeviceType getDeviceType() const noexcept;
    
    /**
     * @brief Get device vendor ID
     * @return VendorID enumeration value
     */
    [[nodiscard]] VendorID getVendorID() const noexcept;
    
    /**
     * @brief Get key hardware resource limits as uint32 values
     * @return Vector of limit values
     */
    [[nodiscard]] std::vector<uint32_t> getResourceLimits() const;
    
    /**
     * @brief Get maximum memory allocation size supported
     * @return Maximum allocation size in bytes
     */
    [[nodiscard]] size_t getMaxAllocationSize() const noexcept;
    
    /**
     * @brief Get maximum sparse memory allocation size supported
     * @return Maximum sparse allocation size in bytes
     */
    [[nodiscard]] size_t getSparseAllocationSize() const noexcept;
    
    /**
     * @brief Check if sparse binding is supported
     * @return True if sparse binding is supported
     */
    [[nodiscard]] bool supportsSparseBinding() const noexcept;
    
    /**
     * @brief Check if sparse residency is supported
     * @return True if sparse residency is supported
     */
    [[nodiscard]] bool supportsSparseResidency() const noexcept;
    
    /**
     * @brief Check if sparse residency for aliased resources is supported
     * @return True if sparse residency aliased is supported
     */
    [[nodiscard]] bool supportsSparseResidencyAliased() const noexcept;

    /**
     * @brief Get device name
     * @return Device name string
     */
    [[nodiscard]] std::string getDeviceName() const;

private:
    void initializeFeatures();
    void initializeProperties();
    void initializeExtensions();

    VkPhysicalDevice m_physical_device;
    Features m_features;
    Properties m_properties;
    std::vector<VkExtensionProperties> m_extensions;
    
    // Cache common device info
    static const std::unordered_map<uint32_t, VendorID> s_vendor_map;
};

} // namespace runtime

#endif // DEVICE_FEATURES_H