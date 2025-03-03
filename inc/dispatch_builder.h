#pragma once
#include <vulkan/vulkan.h>
#include <memory>

namespace runtime {

class ComputeProgram;
class Buffer;

class DispatchBuilder {
public:
    explicit DispatchBuilder(std::shared_ptr<ComputeProgram> program);

    // Workgroup configuration
    DispatchBuilder& setWorkgroupSize(uint32_t x, uint32_t y = 1, uint32_t z = 1);
    DispatchBuilder& setWorkgroupCount(uint32_t x, uint32_t y = 1, uint32_t z = 1);
    
    // Resource binding
    DispatchBuilder& bindStorageBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);
    DispatchBuilder& bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);
    
    // Indirect dispatch
    DispatchBuilder& setIndirectBuffer(std::shared_ptr<Buffer> buffer, VkDeviceSize offset = 0);

    // Build and dispatch
    void dispatch();

private:
    std::shared_ptr<ComputeProgram> m_program;
    uint32_t m_workgroupSizeX{1}, m_workgroupSizeY{1}, m_workgroupSizeZ{1};
    uint32_t m_workgroupCountX{1}, m_workgroupCountY{1}, m_workgroupCountZ{1};
    std::shared_ptr<Buffer> m_indirectBuffer;
    VkDeviceSize m_indirectOffset{0};
    bool m_useIndirect{false};
};

} // namespace runtime
