//#include "vkrt.h"
//
#include "runtime.h"
#include <iostream>
#include <vector>
#include "square.h"

int main()
{
    auto app = runtime::Runtime::create();
    size_t size = static_cast<size_t>(1024 * 1024) * 1024;
    std::vector<uint32_t> data(size/1024, 0);
    for (auto i = 0; i < data.size(); ++i)
        data[i] = i;
    auto dev0 = app->getDevice(0);
    auto dev1 = app->getDevice(1);
    auto buf0 = dev1->createWorkingBuffer(size);
    auto buf1 = dev1->createWorkingBuffer(size);
    auto ptr = dev0->createWorkingBuffer(data.size() * sizeof uint32_t, 0);
    
    std::vector<uint32_t> code(square, square + (sizeof(square) / sizeof(uint32_t)));
       
    auto pgrm = dev1->createProgram(code, 1024);
    pgrm->Arg(buf0, 0, 0);
    pgrm->Arg(buf1, 1, 0);
    auto q = dev1->getComputePoolManager(0);
    auto q1 = dev1->getComputePoolManager(1);
    pgrm->setup(q);
    pgrm->wait();
    return 0;
}
    //
//int main() {
//	vkrt::Instance inst;
//	auto usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
//	auto& dev0 = inst.get_device(0);
//	auto& dev1 = inst.get_device(1);
//
//
//
//	auto& buf = vkrt::StorageBuffer(dev1, 65536, usageFlags);
//	auto& buf1 = vkrt::StorageBuffer(dev1, 65536, usageFlags);
//
//	auto& buf2 = vkrt::StorageBuffer(dev1, 1024, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
//
//
//	std::cout << buf.isLive() << std::endl;
//	std::cout << buf2.isLive() << std::endl;
//	return 0;
//}  // main