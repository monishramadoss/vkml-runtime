#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <string_view>
#include "../error_handling.h"

namespace runtime {

class Device;
class Buffer;

class ComputeProgram {
public:
    struct ProgramCreateInfo {
        std::vector<uint32_t> spirvCode;
        std::string_view entryPoint{"main"};
        uint32_t localSizeX{1};
        uint32_t localSizeY{1};
        uint32_t localSizeZ{1};
    };

    static std::shared_ptr<ComputeProgram> create(
        Device& device,
        const ProgramCreateInfo& createInfo);

    ~ComputeProgram();

    // Prevent copying
    ComputeProgram(const ComputeProgram&) = delete;
    ComputeProgram& operator=(const ComputeProgram&) = delete;

    // Resource binding
    void bindStorageBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);
    void bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);
    
    // Pipeline operations
    void dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
    void dispatchIndirect(std::shared_ptr<Buffer> buffer, VkDeviceSize offset = 0);

    // Synchronization
    void wait();

private:
    ComputeProgram(Device& device, const ProgramCreateInfo& createInfo);
    
    void initialize();
    void createDescriptorSets();
    void createPipeline();
    void cleanup();

    Device& m_device;
    ProgramCreateInfo m_createInfo;

    VkShaderModule m_shaderModule{VK_NULL_HANDLE};
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};

    struct DescriptorSetInfo {
        VkDescriptorSetLayout layout;
        VkDescriptorSet set;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
    };
    std::vector<DescriptorSetInfo> m_descriptorSets;
};

} // namespace runtime
