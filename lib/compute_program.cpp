
#include "compute_program.h"

#include <vulkan/vulkan.h>
#include <memory>


namespace runtime {

static std::shared_ptr<ComputeProgram> ComputeProgram::create(
    Device& device,
    const ProgramCreateInfo& createInfo) {
    return std::make_shared<ComputeProgram>(device, createInfo);
}

ComputeProgram::~ComputeProgram() {
    cleanup();
}

void ComputeProgram::bindStorageBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer) {
    // Implement buffer binding
    
}

void ComputeProgram::bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer) {
    // Implement buffer binding

}

void ComputeProgram::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    // Implement dispatch

}

void ComputeProgram::dispatchIndirect(std::shared_ptr<Buffer> buffer, VkDeviceSize offset) {
    // Implement indirect dispatch

}


void ComputeProgram::wait() {
    // Implement wait


}

ComputeProgram::ComputeProgram(Device& device, const ProgramCreateInfo& createInfo)
    : m_device(device)
    , m_createInfo(createInfo) {
    initialize();
}

void ComputeProgram::initialize() {
    // Implement initialization

}

void ComputeProgram::createDescriptorSets() {
    // Implement descriptor set creation

}

void ComputeProgram::createPipeline() {
    // Implement pipeline creation

}

void ComputeProgram::cleanup() {
    // Implement cleanup

}




} // namespace runtime