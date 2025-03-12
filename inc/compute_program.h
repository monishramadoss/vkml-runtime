#ifndef VKML_COMPUTE_PROGRAM_H
#define VKML_COMPUTE_PROGRAM_H

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace runtime {

class Buffer;

class DescriptorManager;
class ShaderModule;

class ComputeProgram {
public:

    friend static std::shared_ptr<ComputeProgram> create(
    VkDevice device,
    const std::shared_ptr<DescriptorManager>& descriptorManager);
   

    // Prevent copying
    ComputeProgram(const ComputeProgram&) = delete;
    ComputeProgram& operator=(const ComputeProgram&) = delete;

    // Base API for compute dispatch
    void setupPipeline(const std::string& shaderPath, const std::string& entryPoint = "main");
    void updateBufferDescriptor(uint32_t binding, std::shared_ptr<Buffer> buffer);
    void updateBufferDescriptors(uint32_t binding, const std::vector<std::shared_ptr<Buffer>>& buffers);
    void updateImageDescriptor(uint32_t binding, VkImageView imageView, VkSampler sampler = VK_NULL_HANDLE);
    void updateImageDescriptors(uint32_t binding, const std::vector<VkImageView>& imageViews, VkSampler sampler = VK_NULL_HANDLE);
    
    // Record commands
    void beginRecording();
    void endRecording();
    
    // Dispatch and synchronization
    void dispatch(uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
    void dispatchIndirect(std::shared_ptr<Buffer> buffer, VkDeviceSize offset = 0);
    
    // Pipeline barriers
    void wait(VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    
    explicit ComputeProgram(
        VkDevice,
        const std::shared_ptr<DescriptorManager>& );    
    ~ComputeProgram();

private:


    VkDevice m_device;
    VkCommandPool m_commandPool;
    std::shared_ptr<DescriptorManager> m_descriptorManager;
    // Shader module and pipeline
    std::unique_ptr<ShaderModule> m_shaderModule;
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    
    // Command buffers
    VkCommandBuffer m_commandBuffer{VK_NULL_HANDLE};
    bool m_isRecording{false};

    // Descriptor sets
    VkDescriptorSetLayout m_descriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorSet m_descriptorSet{VK_NULL_HANDLE};
    
    // Shader info
    std::string m_shaderPath;
    std::string m_entryPoint;
    
    // Resources
    std::unordered_map<uint32_t, std::vector<std::shared_ptr<Buffer>>> m_boundBuffers;


    
    // Protected initialization methods
 
    void cleanup();
    

};

} // namespace runtime

#endif // COMPUTE_PROGRAM_H