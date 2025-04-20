//#include "vkrt.h"
//
#include "runtime.h"
#include <iostream>
#include <vector>
#include "square.h"
#include <stdlib.h> // For setenv on non-Windows
#ifdef _WIN32
#include <stdlib.h> // For _putenv_s on Windows
#endif
#include "device.h"
#include "logging.h"
#include "storage.h"

int main()
{
    // Disable performance counters via environment variables
#ifdef _WIN32
    _putenv_s("VK_LAYER_DISABLE_PERFORMANCE", "1");
#else
    setenv("VK_LAYER_DISABLE_PERFORMANCE", "1", 1);
#endif

    // Create runtime
    std::cout << "Creating runtime..." << std::endl;
    auto app = runtime::Runtime::create();
    std::cout << "Runtime created" << std::endl;
    
    // Use a smaller buffer size to avoid memory issues
    size_t size = static_cast<size_t>(1024 * 1024) * 16; // Reduced from 64MB to 16MB
    std::vector<uint32_t> data(size/sizeof(uint32_t), 0);
    
    std::cout << "Initializing data..." << std::endl;
    for (auto i = 0; i < data.size(); ++i)
        data[i] = i;
        
    // Try to get device 0 first, and check if it exists
    std::cout << "Device count: " << app->deviceCount() << std::endl;
    
    if (app->deviceCount() == 0) {
        std::cerr << "No Vulkan devices found!" << std::endl;
        return 1;
    }
      // Use device 0 only to avoid multi-device issues
    std::cout << "Getting device..." << std::endl;
    auto dev0 = app->getDevice(0, {1, 1});
    auto dev1 = dev0; // Use the same device
    
    std::cout << "Creating buffers..." << std::endl;
    auto buf0 = dev1->createWorkingBuffer(size);
    if (!buf0) {
        std::cerr << "Failed to create input buffer!" << std::endl;
        return 1;
    }
    
    auto buf1 = dev1->createWorkingBuffer(size);
    if (!buf1) {
        std::cerr << "Failed to create output buffer!" << std::endl;
        return 1;
    }

    std::cout << "Getting pointers..." << std::endl;
    auto dPtr = (float *)buf0->getPtr();
    if (!dPtr) {
        std::cerr << "Failed to get input buffer pointer! Buffer memory may not be host-visible." << std::endl;
        std::cerr << "Try using a different device or memory type." << std::endl;
        return 1;
    }
    
    auto *dPtr1 = (float *)buf1->getPtr();
    if (!dPtr1) {
        std::cerr << "Failed to get output buffer pointer! Buffer memory may not be host-visible." << std::endl;
        std::cerr << "Try using a different device or memory type." << std::endl;
        return 1;
    }

    std::cout << "Initializing input data..." << std::endl;
    try {
        for (auto i = 0; i < data.size(); ++i)
            dPtr[i] = static_cast<float>(data[i]);
    } catch (const std::exception& e) {
        std::cerr << "Error while writing to buffer: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Setting up shader code..." << std::endl;
    std::vector<uint32_t> code(square, square + (sizeof(square) / sizeof(uint32_t)));
       
    std::cout << "Creating program..." << std::endl;
    auto pgrm = dev1->createProgram(code, 1024);
    if (!pgrm) {
        std::cerr << "Failed to create program!" << std::endl;
        return 1;
    }
    
    std::cout << "Setting program arguments..." << std::endl;
    try {
        pgrm->Arg(buf0, 0, 0);
        pgrm->Arg(buf1, 1, 0);
    } catch (const std::exception& e) {
        std::cerr << "Error setting program arguments: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Getting compute pool manager..." << std::endl;
    auto c = dev0->getComputePoolManager(0, VK_QUEUE_COMPUTE_BIT);
    if (!c) {
        std::cerr << "Failed to get compute pool manager!" << std::endl;
        return 1;
    }
    
    // Set up transfer pool manager with error checking
    auto t = dev1->getComputePoolManager(0, VK_QUEUE_TRANSFER_BIT);
    
    std::cout << "Setting up program..." << std::endl;
    try {
        pgrm->setup(c);
    } catch (const std::exception& e) {
        std::cerr << "Error setting up program: " << e.what() << std::endl;
        return 1;
    }
 
    std::cout << "Submitting and waiting..." << std::endl;
    try {
        dev1->submit({c}, 0);
        c->wait();
    } catch (const std::exception& e) {
        std::cerr << "Error during execution: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Execution completed!" << std::endl;std::cout << "Results: ";
    for (auto i = 0; i < 10; ++i)
        std::cout << dPtr1[i] << " ";
    std::cout << std::endl << std::flush;
    return 0;
}

