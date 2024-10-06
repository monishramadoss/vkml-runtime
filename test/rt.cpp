#include "vk_instance.h"


int main() {
	vkrt::Instance inst;
	auto usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	auto& dev0 = inst.get_device(0);
	auto& dev1 = inst.get_device(1);
	auto& buf = vkrt::Buffer(dev0, 65536, usageFlags);
	auto& buf2 = vkrt::Buffer(dev1, 1024, usageFlags);

	return 0;
}