#pragma once
#include <vector>
#include "compute/compute_program.h"
#include "compute/shader_module.h"
#include "compute/dispatch_builder.h"
#include "compute/compute_fence.h"

namespace runtime {

// Convenience aliases
using ComputeProgramPtr = std::shared_ptr<ComputeProgram>;
using ShaderModulePtr = std::shared_ptr<ShaderModule>;
using DispatchBuilderPtr = std::shared_ptr<DispatchBuilder>;
using ComputeFencePtr = std::shared_ptr<ComputeFence>;

// Factory functions
inline ComputeProgramPtr createComputeProgram(
    Device& device,
    const ComputeProgram::ProgramCreateInfo& createInfo) {
    return ComputeProgram::create(device, createInfo);
}

inline ShaderModulePtr createShaderModule(
    Device& device,
    const std::vector<uint32_t>& spirvCode) {
    return ShaderModule::create(device, spirvCode);
}

inline DispatchBuilderPtr createDispatchBuilder(ComputeProgramPtr program) {
    return std::make_shared<DispatchBuilder>(program);
}

inline ComputeFencePtr createComputeFence(Device& device) {
    return std::make_shared<ComputeFence>(device);
}

class ComputeProgram
{
    Device *m_dev;
    std::shared_ptr<vkrt_compute_program> m_program;
  public:
    ComputeProgram(Device &dev, const std::vector<uint32_t> &code);
    void record(uint32_t x, uint32_t y, uint32_t z);
    void record(uint32_t x) { record(x, 1, 1); }
    void record(uint32_t x, uint32_t y){ record(x, y, 1);}

    template <typename... T> void operator()(T &...args)
    {
        *this(0, args...);
    }
    template <typename... T> void operator()(size_t i, T &...args);
};



ComputeProgram::ComputeProgram(Device &dev, const std::vector<uint32_t> &code)
{
    m_dev = &dev;
    m_program = m_dev->construct(code);
}



inline void ComputeProgram::record(uint32_t x, uint32_t y, uint32_t z)
{
    ComputePacket packet{};
    packet.flags = VK_SHADER_STAGE_COMPUTE_BIT;
    packet.dims[0] = x;
    packet.dims[1] = y;
    packet.dims[2] = z;
    packet.pipeline = m_program->pipeline;
    packet.layout = m_program->pipeline_layout;
    packet.sets = m_program->sets;
    packet.n_sets = m_program->n_sets;
    m_dev->submit(packet);
}
//
//template<typename T...>
//void ComputerProgram::operator()(size_t i, T &...args)
//{
//   m_dev->update(i, m_program, );
//    
//
#pragma once
#include <vector>
#include <memory>
#include "compute/compute_program.h"
#include "compute/shader_module.h"
#include "compute/dispatch_builder.h"
#include "compute/compute_fence.h"
#include "buffer/buffer.h"
#include "error_handling.h"

namespace runtime {

// Forward declarations
class Device;
struct ComputePacket;

// Convenience aliases
using ComputeProgramPtr = std::shared_ptr<ComputeProgram>;
using ShaderModulePtr = std::shared_ptr<ShaderModule>;
using DispatchBuilderPtr = std::shared_ptr<DispatchBuilder>;
using ComputeFencePtr = std::shared_ptr<ComputeFence>;
using BufferPtr = std::shared_ptr<Buffer>;

class ComputeProgram {
public:
    explicit ComputeProgram(Device& device, const std::vector<uint32_t>& code)
        : m_device(&device)
        , m_program(device.construct(code)) {}

    // Prevent copying
    ComputeProgram(const ComputeProgram&) = delete;
    ComputeProgram& operator=(const ComputeProgram&) = delete;

    // Enable moving
    ComputeProgram(ComputeProgram&&) noexcept = default;
    ComputeProgram& operator=(ComputeProgram&&) noexcept = default;

    // Dispatch methods
    void record(uint32_t x, uint32_t y = 1, uint32_t z = 1) {
        ComputePacket packet{};
        packet.flags = VK_SHADER_STAGE_COMPUTE_BIT;
        packet.dims[0] = x;
        packet.dims[1] = y;
        packet.dims[2] = z;
        packet.pipeline = m_program->pipeline;
        packet.layout = m_program->pipeline_layout;
        packet.sets = m_program->sets;
        packet.n_sets = m_program->n_sets;
        m_device->submit(packet);
    }

    // Variadic template for binding buffers
    template<typename... Buffers>
    void operator()(size_t set_index, const Buffers&... buffers) {
        if constexpr (sizeof...(buffers) > 0) {
            std::vector<std::shared_ptr<vkrt_buffer>> buffer_list;
            (buffer_list.push_back(buffers), ...);
            m_device->update(set_index, m_program, buffer_list);
            record(1); // Default dispatch if not specified
        }
    }

    // Getter methods
    auto& getProgram() const { return m_program; }
    Device& getDevice() const { return *m_device; }

private:
    Device* m_device;
    std::shared_ptr<vkrt_compute_program> m_program;
};

// Factory functions
inline ComputeProgramPtr createComputeProgram(Device& device, 
                                            const std::vector<uint32_t>& spirvCode) {
    return std::make_shared<ComputeProgram>(device, spirvCode);
}

inline ShaderModulePtr createShaderModule(Device& device, 
                                        const std::vector<uint32_t>& spirvCode) {
    return ShaderModule::create(device, spirvCode);
}

inline DispatchBuilderPtr createDispatchBuilder(const ComputeProgramPtr& program) {
    return std::make_shared<DispatchBuilder>(program);
}

inline ComputeFencePtr createComputeFence(Device& device) {
    return std::make_shared<ComputeFence>(device);
}

// Utility functions
template<typename... Args>
void dispatchCompute(ComputeProgramPtr program, uint32_t x, uint32_t y, uint32_t z, Args&&... args) {
    program->operator()(0, std::forward<Args>(args)...);
    program->record(x, y, z);
}

template<typename... Args>
void dispatchCompute(ComputeProgramPtr program, uint32_t x, Args&&... args) {
    dispatchCompute(program, x, 1, 1, std::forward<Args>(args)...);
}

} // namespace runtime
//}

} // namespace runtime