#include "test_utils.h"
#include "storage.h"
#include "device.h"
#include "logging.h"
#include "runtime.h"
#include <vector>
#include <cstring>
#include <iostream>

using namespace runtime;
using namespace vkrt::test;

class StorageTestBase : public Test {
public:
    StorageTestBase(std::string name) : Test(name) {}
    void setup() override {
        runtime = Runtime::create();
        if (runtime && runtime->deviceCount() > 0) {
            auto devices = runtime->pullDevices();
            if (!devices.empty())
                device = devices[0];
        }
    }
    void teardown() override {
        device.reset();
        runtime.reset();
    }
protected:
    std::shared_ptr<Runtime> runtime;
    std::shared_ptr<Device> device;
};

class BufferAllocationTest : public StorageTestBase {
public:
    BufferAllocationTest(std::string name) : StorageTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        size_t bufferSize = 1024;
        auto buffer = device->createWorkingBuffer(bufferSize);
        TEST_ASSERT(buffer != nullptr, "Buffer allocation failed");
    }
};
REGISTER_TEST(BufferAllocationTest);

class BufferMappingTest : public StorageTestBase {
public:
    BufferMappingTest(std::string name) : StorageTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        size_t bufferSize = sizeof(float) * 10;
        auto buffer = device->createWorkingBuffer(bufferSize);
        TEST_ASSERT(buffer != nullptr, "Buffer allocation failed");
        std::vector<float> testData(10, 42.0f);
        // Use copyDataFrom/copyDataTo instead of map/unmap
        buffer->copyDataFrom(testData.data(), bufferSize, 0);
        std::vector<float> resultData(10, 0.0f);
        buffer->copyDataTo(resultData.data(), bufferSize, 0);
        for (int i = 0; i < 10; i++) {
            TEST_ASSERT(resultData[i] == 42.0f, "Data verification failed at index " + std::to_string(i));
        }
    }
};
REGISTER_TEST(BufferMappingTest);

class BufferTransferTest : public StorageTestBase {
public:
    BufferTransferTest(std::string name) : StorageTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        size_t bufferSize = sizeof(float) * 10;
        auto srcBuffer = device->createSrcTransferBuffer(bufferSize);
        auto dstBuffer = device->createDstTransferBuffer(bufferSize);
        TEST_ASSERT(srcBuffer != nullptr, "Source buffer allocation failed");
        TEST_ASSERT(dstBuffer != nullptr, "Destination buffer allocation failed");
        std::vector<float> testData(10, 42.0f);
        srcBuffer->copyDataFrom(testData.data(), bufferSize, 0);
        // Use copyDataTo to transfer data from srcBuffer to dstBuffer
        srcBuffer->copyDataTo(dstBuffer, bufferSize, 0, 0);
        std::vector<float> resultData(10, 0.0f);
        dstBuffer->copyDataTo(resultData.data(), bufferSize, 0);
        for (int i = 0; i < 10; i++) {
            TEST_ASSERT(resultData[i] == 42.0f, "Data transfer verification failed at index " + std::to_string(i));
        }
    }
};
REGISTER_TEST(BufferTransferTest);
