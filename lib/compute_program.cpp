#include "compute_program.h"
#include "device.h"
#include "buffer.h"
#include "descriptor_manager.h"
#include "error_handling.h"
#include "shader_module.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace runtime {

static std::shared_ptr<ComputeProgram> create(
        VkDevice device,
        const std::shared_ptr<DescriptorManager>& descriptorManager) {
            return std::shared_ptr<ComputeProgram>(new ComputeProgram(device, descriptorManager));
    }

    
ComputeProgram::ComputeProgram(
    VkDevice device,
    const std::shared_ptr<DescriptorManager>& descriptorManager)
    : m_device(device)
    , m_descriptorManager(descriptorManager) {
    
}
ComputeProgram::~ComputeProgram() {

}

} // namespace runtime