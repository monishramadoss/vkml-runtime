#include "dispatch_builder.h"
#include "barrier.h"
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
        if (!buffer) {
            throw std::invalid_argument("Null buffer provided to bindStorageBuffer");
        }
        
        if (!m_program) {
            throw std::runtime_error("No compute program set in dispatch builder");
        }
        
        ErrorCode result = m_program->bindStorageBuffer(set, binding, buffer);
        if (result != ErrorCode::SUCCESS) {
            throw std::runtime_error("Failed to bind storage buffer: " + std::to_string(static_cast<int>(result)));
        }
        
        return *this;
    }

    DispatchBuilder& DispatchBuilder::bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer)
    {
        if (!buffer) {
            throw std::invalid_argument("Null buffer provided to bindUniformBuffer");
        }
        
        if (!m_program) {
            throw std::runtime_error("No compute program set in dispatch builder");
        }
        
        ErrorCode result = m_program->bindUniformBuffer(set, binding, buffer);
        if (result != ErrorCode::SUCCESS) {
            throw std::runtime_error("Failed to bind uniform buffer: " + std::to_string(static_cast<int>(result)));
        }
        
        return *this;
    }

    DispatchBuilder& DispatchBuilder::bindTexture(uint32_t set, uint32_t binding, std::shared_ptr<Texture> texture, 
                                                 BindingType type)
    {
        if (!texture) {
            throw std::invalid_argument("Null texture provided to bindTexture");
        }
        
        if (!m_program) {
            throw std::runtime_error("No compute program set in dispatch builder");
        }
        
        ErrorCode result = m_program->bindTexture(set, binding, texture, type);
        if (result != ErrorCode::SUCCESS) {
            throw std::runtime_error("Failed to bind texture: " + std::to_string(static_cast<int>(result)));
        }
        
        return *this;
    }

    DispatchBuilder& DispatchBuilder::setIndirectBuffer(std::shared_ptr<Buffer> buffer, VkDeviceSize offset)
    {
        if (!buffer) {
            throw std::invalid_argument("Null buffer provided to setIndirectBuffer");
        }
        
        m_indirectBuffer = buffer;
        m_indirectOffset = offset;
        m_useIndirect = true;
        return *this;
    }

    DispatchBuilder& DispatchBuilder::setPushConstants(const void* data, uint32_t size, uint32_t offset)
    {
        if (!m_program) {
            throw std::runtime_error("No compute program set in dispatch builder");
        }
        
        m_pushConstantData = data;
        m_pushConstantSize = size;
        m_pushConstantOffset = offset;
        m_usePushConstants = true;
        
        return *this;
    }

    void DispatchBuilder::dispatch()
    {
        if (!m_program) {
            throw std::runtime_error("Cannot dispatch: no compute program set");
        }
        
        // Apply push constants if needed
        if (m_usePushConstants && m_pushConstantData && m_pushConstantSize > 0) {
            ErrorCode result = m_program->pushConstants(m_pushConstantData, m_pushConstantSize, m_pushConstantOffset);
            if (result != ErrorCode::SUCCESS) {
                throw std::runtime_error("Failed to set push constants: " + std::to_string(static_cast<int>(result)));
            }
        }
        
        // Dispatch compute work
        ErrorCode result;
        if (m_useIndirect) {
            if (!m_indirectBuffer) {
                throw std::runtime_error("Indirect dispatch requested but no indirect buffer provided");
            }
            
            result = m_program->dispatchIndirect(m_indirectBuffer, m_indirectOffset);
        } else {
            // Validate workgroup counts
            if (m_workgroupCountX == 0 || m_workgroupCountY == 0 || m_workgroupCountZ == 0) {
                throw std::runtime_error("Invalid workgroup count: cannot be zero");
            }
            
            result = m_program->dispatch(m_workgroupCountX, m_workgroupCountY, m_workgroupCountZ);
        }
        
        if (result != ErrorCode::SUCCESS) {
            throw std::runtime_error("Dispatch failed: " + std::to_string(static_cast<int>(result)));
        }
    }

    void DispatchBuilder::wait(uint64_t timeout)
    {
        if (!m_program) {
            throw std::runtime_error("No compute program set");
        }
        
        ErrorCode result = m_program->wait(timeout);
        if (result != ErrorCode::SUCCESS && result != ErrorCode::TIMEOUT) {
            throw std::runtime_error("Wait failed: " + std::to_string(static_cast<int>(result)));
        }
    }

    bool DispatchBuilder::isCompleted() const
    {
        if (!m_program) {
            throw std::runtime_error("No compute program set");
        }
        
        return m_program->isCompleted();
    }

} // namespace runtime