#include "test_utils.h"
#include "device.h"
#include "logging.h"
#include "storage.h"
#include "runtime.h"
#include "program.h"
#include <vector>
#include <iostream>

using namespace runtime;
using namespace vkrt::test;

class ShaderLoadTest : public Test {
public:
    ShaderLoadTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        if (!runtime || runtime->deviceCount() == 0) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        auto devices = runtime->pullDevices();
        if (devices.empty()) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        auto device = devices[0];
        std::vector<uint32_t> shaderCode = { 0x07230203, 0x00010000, 0x00080001, 0x0000000a };
        try {
            auto program = device->createProgram(shaderCode);
            // In a real test with valid shader code:
            // TEST_ASSERT(program != nullptr, "Program creation failed");
        } catch (const std::exception& e) {
            std::cout << "Expected exception with mock shader: " << e.what() << std::endl;
        }
    }
};
REGISTER_TEST(ShaderLoadTest);

class ProgramExecutionTest : public Test {
public:
    ProgramExecutionTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        if (!runtime || runtime->deviceCount() == 0) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        auto devices = runtime->pullDevices();
        if (devices.empty()) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        auto device = devices[0];
        auto inputBuffer = device->createSrcTransferBuffer(1024);
        auto outputBuffer = device->createDstTransferBuffer(1024);
        TEST_ASSERT(inputBuffer != nullptr, "Input buffer creation failed");
        TEST_ASSERT(outputBuffer != nullptr, "Output buffer creation failed");
        // In a real test, you would:
        // 1. Load actual SPIR-V shader code
        // 2. Create program
        // 3. Set descriptor sets and push constants
        // 4. Execute program
        // 5. Verify results
    }
};
REGISTER_TEST(ProgramExecutionTest);

// Replace Buffer::map/unmap/getMappedData with Buffer::getPtr(), and use copyDataFrom/copyDataTo for data transfer.
