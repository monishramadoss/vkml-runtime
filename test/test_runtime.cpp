#include "test_utils.h"
#include "runtime.h"
#include "device.h"
#include "logging.h"
#include "storage.h"
#include <memory>
#include <vector>
#include <iostream>

using namespace runtime;
using namespace vkrt::test;

class RuntimeCreationTest : public Test {
public:
    RuntimeCreationTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        TEST_ASSERT(runtime->getInstance() != VK_NULL_HANDLE, "Runtime instance is null");
    }
};
REGISTER_TEST(RuntimeCreationTest);

class DeviceCountTest : public Test {
public:
    DeviceCountTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        TEST_ASSERT(runtime->deviceCount() > 0, "No devices found");
    }
};
REGISTER_TEST(DeviceCountTest);

class GetDeviceTest : public Test {
public:
    GetDeviceTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        if (runtime->deviceCount() > 0) {
            auto devices = runtime->pullDevices();
            if (!devices.empty()) {
                auto device = devices[0];
                TEST_ASSERT(device != nullptr, "Device is null");
                TEST_ASSERT(device->getDevice() != VK_NULL_HANDLE, "Device handle is null");
            }
        }
    }
};
REGISTER_TEST(GetDeviceTest);

class PullDevicesTest : public Test {
public:
    PullDevicesTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        auto devices = runtime->pullDevices();
        TEST_ASSERT(devices.size() == runtime->deviceCount(), "Device count mismatch");
        for (const auto& device : devices) {
            TEST_ASSERT(device != nullptr, "Device is null");
            TEST_ASSERT(device->getDevice() != VK_NULL_HANDLE, "Device handle is null");
        }
    }
};
REGISTER_TEST(PullDevicesTest);

class BufferCreationTest : public Test {
public:
    BufferCreationTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        if (runtime->deviceCount() > 0) {
            auto hostBuffer = runtime->createHostBuffer(1024, 0);
            TEST_ASSERT(hostBuffer != nullptr, "Host buffer creation failed");
            auto deviceBuffer = runtime->createDeviceBuffer(1024, 0);
            TEST_ASSERT(deviceBuffer != nullptr, "Device buffer creation failed");
        }
    }
};
REGISTER_TEST(BufferCreationTest);

class DataCopyTest : public Test {
public:
    DataCopyTest(std::string name) : Test(name) {}
    void run() override {
        auto runtime = Runtime::create();
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        if (runtime->deviceCount() > 0) {
            auto hostBuffer = runtime->createHostBuffer(sizeof(float) * 10, 0);
            auto deviceBuffer = runtime->createDeviceBuffer(sizeof(float) * 10, 0);
            TEST_ASSERT(hostBuffer != nullptr, "Host buffer creation failed");
            TEST_ASSERT(deviceBuffer != nullptr, "Device buffer creation failed");
            std::vector<float> testData(10, 42.0f);
            runtime->copyData(testData.data(), deviceBuffer, sizeof(float) * 10, 0, 0);
            std::vector<float> resultData(10, 0.0f);
            runtime->copyData(deviceBuffer, resultData.data(), sizeof(float) * 10, 0, 0);
            for (int i = 0; i < 10; i++) {
                TEST_ASSERT(resultData[i] == 42.0f, "Data copy verification failed at index " + std::to_string(i));
            }
        }
    }
};
REGISTER_TEST(DataCopyTest);

// Replace Buffer::map/unmap/getMappedData with Buffer::getPtr(), and use copyDataFrom/copyDataTo for data transfer.
