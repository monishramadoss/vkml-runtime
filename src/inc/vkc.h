#pragma once
#include <vector>

namespace runtime {


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
//}

} // namespace vkrt