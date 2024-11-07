#include "square.h"
#include "vk_instance.h"
#include <iostream>
#include <vector>

// glslangValidator.exe --target-env vulkan1.3 --vn square -o .\test\square.h .\test\square.comp .\test\square.comp

int main()
{
    vkrt::Instance inst;
    auto &dev0 = inst.get_device(0);
    auto &dev1 = inst.get_device(1);
    auto size = 1 << 10;
    auto buf = vkrt::StorageBuffer(dev1, size * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
    auto buf1 = vkrt::StorageBuffer(dev1, size * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);

    std::vector<float> data(size, 2.0f);
    std::vector<float> data1(size);
    std::memcpy(buf.data(), data.data(), size * sizeof(float));
    std::vector<uint32_t> code(square, square + (sizeof(square) / sizeof(uint32_t)));

    // auto& upload = vkrt::SendBuffer(dev1, data, buf);
    // auto& download = vkrt::RecvBuffer(dev1, data1, buf1);

    std::vector<vkrt::StorageBuffer> bufs = {buf, buf1};

    auto cpt = vkrt::Program(dev1, code, bufs);
    cpt.record(size / 1024, 1, 1);

    return 0;
}
