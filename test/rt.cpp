#include "vkrt.h"



int main() {
	vkrt::Instance inst;
	auto usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	auto& dev0 = inst.get_device(0);
	auto& dev1 = inst.get_device(1);



	auto& buf = vkrt::buffer(dev1, 65536, usageFlags);
	auto& buf1 = vkrt::buffer(dev1, 65536, usageFlags);

	auto& buf2 = vkrt::buffer(dev1, 1024, VK_BUFFER_USAGE_TRANSFER_DST_BIT);


	std::cout << buf.isLive() << std::endl;
	std::cout << buf2.isLive() << std::endl;
	return 0;
}  // main