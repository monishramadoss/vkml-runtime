#ifndef SHADER_MODULE_H
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "error_handling.h"

namespace runtime {
class ShaderModule {
public:
    static std::shared_ptr<ShaderModule> create(
        VkDevice device,
        const std::vector<uint32_t>& spirvCode);

    ~ShaderModule();

    // Prevent copying
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    VkShaderModule getModule() const { return m_module; }

    // Reflection info
    struct ResourceBinding {
        uint32_t set;
        uint32_t binding;
        VkDescriptorType descriptorType;
        VkShaderStageFlags stageFlags;
        uint32_t descriptorCount;
    };

    const std::vector<ResourceBinding>& getResourceBindings() const { return m_resourceBindings; }

private:
    ShaderModule(VkDevice device, const std::vector<uint32_t>& spirvCode);
    
    void initialize();
    void parseReflectionData();
    void cleanup();

    VkDevice m_device;
    std::vector<uint32_t> m_spirvCode;
    VkShaderModule m_module{VK_NULL_HANDLE};
    std::vector<ResourceBinding> m_resourceBindings;
};

} // namespace runtime

#endif // SHADER_MODULE_H