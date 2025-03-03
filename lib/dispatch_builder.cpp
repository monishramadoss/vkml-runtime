#include "dispatch_builder.h"
#include "compute_fence.h"
#include "compute_program.h"
#include "device.h"
#include "buffer.h"
#include "shader_module.h"
#include "error_handling.h"

namespace runtime {

    DispatchBuilder::DispatchBuilder(std::shared_ptr<ComputeProgram> program)
        : m_program(program)
    {
    }

    DispatchBuilder& DispatchBuilder::setWorkgroupSize(uint32_t x, uint32_t y, uint32_t z)
    {
        m_workgroupSizeX = x;
        m_workgroupSizeY = y;
        m_workgroupSizeZ = z;
        return *this;
    }

    DispatchBuilder& DispatchBuilder::setWorkgroupCount(uint32_t x, uint32_t y, uint32_t z)
    {
        m_workgroupCountX = x;
        m_workgroupCountY = y;
        m_workgroupCountZ = z;
        return *this;
    }

    DispatchBuilder& DispatchBuilder::bindStorageBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer)
    {
        m_program->bindStorageBuffer(set, binding, buffer);
        return *this;
    }

    DispatchBuilder& DispatchBuilder::bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer)
    {
        m_program->bindUniformBuffer(set, binding, buffer);
        return *this;
    }

    DispatchBuilder& DispatchBuilder::setIndirectBuffer(std::shared_ptr<Buffer> buffer, VkDeviceSize offset)
    {
        m_indirectBuffer = buffer;
        m_indirectOffset = offset;
        m_useIndirect = true;
        return *this;
    }

    void DispatchBuilder::dispatch()
    {
        if (m_useIndirect) {
            m_program->dispatchIndirect(m_indirectBuffer, m_indirectOffset);
        } else {
            m_program->dispatch(m_workgroupCountX, m_workgroupCountY, m_workgroupCountZ);
        }
    }

} // namespace runtime