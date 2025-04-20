#ifndef PROGRAM_H
#define PROGRAM_H

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

#include <vector>
#include <memory>

#ifndef SPIRV_REFLECT_INC_H
#define SPIRV_REFLECT_INC_H
#include <spirv_reflect.h>
#endif // SPIRV_CROSS_INC_H


namespace runtime
{
class Buffer;
class Device;
class DescriptorAllocator;
class DescriptorLayoutCache;
class CommandPoolManager;

class Program
{
  public:
    static std::shared_ptr<Program> create(VkDevice device, VkPipelineCache pipeline_cache,
                                           std::shared_ptr<DescriptorLayoutCache> &descCache,
                                           std::shared_ptr<DescriptorAllocator> &descAllocator,                                          
                                           const std::vector<uint32_t> &shader_code, uint32_t dim_x, uint32_t dim_y,
                                           uint32_t dim_z);
    

    Program(VkDevice device, VkPipelineCache pipeline_cache, 
            std::shared_ptr<DescriptorLayoutCache> &descCache,
            std::shared_ptr<DescriptorAllocator> &descAllocator,          
            const std::vector<uint32_t> &shader_code,
            uint32_t dim_x, uint32_t dim_y, uint32_t dim_z);

    ~Program();
    void Arg(std::shared_ptr<Buffer> &buffer, size_t binding_idx = 0,size_t set_idx = 0);
    void setup(std::shared_ptr<CommandPoolManager> cmd_pool);
  
  private:
    void initialize(VkDevice device, VkPipelineCache pipeline_cache,
                    std::shared_ptr<DescriptorLayoutCache> &descCache,
                    std::shared_ptr<DescriptorAllocator> &descAllocator,  
                    const std::vector<uint32_t> &shader_code);
    void cleanup();

    VkDevice m_device;
    VkShaderModule m_module;
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
    uint32_t dims[3]{0, 0, 0};
    std::vector<VkDescriptorSet> sets; 
    std::vector<std::vector<VkWriteDescriptorSet>> writes;
    std::shared_ptr<CommandPoolManager> m_cmdPoolManager;
};

} // namespace runtime


#endif // PROGRAM_H