<<<<<<< HEAD
#include "vk_instance.h"
=======
#include "square.h"
#include "vkrt.h"
#include <iostream>
>>>>>>> 7fc3dd2 (seventh commit)
#include <vector>
#include <iostream>
#include "square.h"

// glslangValidator.exe --target-env vulkan1.3 --vn square -o .\test\square.h .\test\square.comp .\test\square.comp


int main() {
	vkrt::Instance inst;
	auto& dev0 = inst.get_device(0);
	auto& dev1 = inst.get_device(1);
    auto size = 1 << 10;
	auto buf = vkrt::StorageBuffer(dev1, size * sizeof (float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT , true);
	auto buf1 = vkrt::StorageBuffer(dev1, size * sizeof (float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);

<<<<<<< HEAD

	std::vector <float> data (size, 2.0f);
    std::vector <float> data1 (size);
	std::memcpy(buf.data(), data.data(), size * sizeof(float));
	std::vector<uint32_t> code(square, square + (sizeof(square) / sizeof(uint32_t)));

	//auto& upload = vkrt::SendBuffer(dev1, data, buf);
    //auto& download = vkrt::RecvBuffer(dev1, data1, buf1);
=======
    std::vector<float> input_data(size, 2.f);
    std::memcpy(buf.data(), input_data.data(), size * sizeof(float));
    std::vector<uint32_t> code(square, square + (sizeof(square) / sizeof(uint32_t)));
      
    std::vector<vkrt::StorageBuffer> bufs = {buf, buf1};
>>>>>>> 7fc3dd2 (seventh commit)

	std::vector<vkrt::StorageBuffer> bufs = { buf, buf1 };

	auto cpt = vkrt::Program(dev1, code, bufs);
	cpt.record(size / 1024, 1, 1);


    std::vector<float> output_data(size, 0.f);  
    std::memcpy(output_data.data(), buf1.data(), size * sizeof(float));
    for (auto i = 0; i < size; ++i)
    {
        if (output_data[i] != 4.f)
        {
            std::cout << "ERROR: " << i << " " << output_data[i] << std::endl;
            break;
        }
    }
    return 0;
}

