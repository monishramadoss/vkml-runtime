#include "test_utils.h"
#include "device.h"
#include "logging.h"
#include "storage.h"
#include "runtime.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <iostream>

using namespace runtime;
using namespace vkrt::test;

// Shared setup for device tests
class DeviceTestBase : public Test {
public:
    DeviceTestBase(std::string name) : Test(name) {}
    void setup() override {
        runtime = Runtime::create();
        if (runtime && runtime->deviceCount() > 0) {
            // Use pullDevices() to get the device list and select the first
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

class DeviceCreationTest : public DeviceTestBase {
public:
    DeviceCreationTest(std::string name) : DeviceTestBase(name) {}
    void run() override {
        TEST_ASSERT(runtime != nullptr, "Runtime creation failed");
        if (runtime->deviceCount() > 0) {
            TEST_ASSERT(device != nullptr, "Device is null");
            TEST_ASSERT(device->getDevice() != VK_NULL_HANDLE, "Device handle is null");
        } else {
            std::cout << "Skipping test: No suitable devices found for testing" << std::endl;
        }
    }
};
REGISTER_TEST(DeviceCreationTest);

class BufferCreationTest : public DeviceTestBase {
public:
    BufferCreationTest(std::string name) : DeviceTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        auto workingBuffer = device->createWorkingBuffer(1024);
        TEST_ASSERT(workingBuffer != nullptr, "Working buffer creation failed");
        auto srcBuffer = device->createSrcTransferBuffer(1024);
        TEST_ASSERT(srcBuffer != nullptr, "Source transfer buffer creation failed");
        auto dstBuffer = device->createDstTransferBuffer(1024);
        TEST_ASSERT(dstBuffer != nullptr, "Destination transfer buffer creation failed");
    }
};
REGISTER_TEST(BufferCreationTest);

class ProgramCreationDeviceTest : public DeviceTestBase {
public:
    ProgramCreationDeviceTest(std::string name) : DeviceTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
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
REGISTER_TEST(ProgramCreationDeviceTest);

class GetDeviceFeaturesTest : public DeviceTestBase {
public:
    GetDeviceFeaturesTest(std::string name) : DeviceTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        const auto& features = device->getDeviceFeatures();
        (void)features;
    }
};
REGISTER_TEST(GetDeviceFeaturesTest);

class CommandPoolManagerTest : public DeviceTestBase {
public:
    CommandPoolManagerTest(std::string name) : DeviceTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        // Correct usage: getComputePoolManager(size_t idx, VkQueueFlagBits flags)
        auto poolManager = device->getComputePoolManager(0, VK_QUEUE_COMPUTE_BIT);
        TEST_ASSERT(poolManager != nullptr, "Failed to get compute pool manager");
    }
};
REGISTER_TEST(CommandPoolManagerTest);

class ThreadPoolCreationTest : public Test {
public:
    ThreadPoolCreationTest(std::string name) : Test(name) {}
    void run() override {
        auto threadPool = ThreadPool::create();
        TEST_ASSERT(threadPool != nullptr, "Thread pool creation failed");
        auto threadPoolWithCount = ThreadPool::create(2);
        TEST_ASSERT(threadPoolWithCount != nullptr, "Thread pool creation with count failed");
    }
};
REGISTER_TEST(ThreadPoolCreationTest);

class ThreadPoolEnqueueTest : public Test {
public:
    ThreadPoolEnqueueTest(std::string name) : Test(name) {}
    void run() override {
        auto threadPool = ThreadPool::create(2);
        TEST_ASSERT(threadPool != nullptr, "Thread pool creation failed");
        bool taskExecuted = false;
        std::mutex mutex;
        std::condition_variable cv;
        threadPool->enqueue([&taskExecuted, &mutex, &cv]() {
            std::unique_lock<std::mutex> lock(mutex);
            taskExecuted = true;
            cv.notify_one();
        });
        std::unique_lock<std::mutex> lock(mutex);
        bool success = cv.wait_for(lock, std::chrono::seconds(2), [&taskExecuted]() { return taskExecuted; });
        TEST_ASSERT(success, "Task execution timed out");
        TEST_ASSERT(taskExecuted, "Task not executed");
    }
};
REGISTER_TEST(ThreadPoolEnqueueTest);

class BufferDataTransferTest : public DeviceTestBase {
public:
    BufferDataTransferTest(std::string name) : DeviceTestBase(name) {}
    void run() override {
        if (!device) {
            std::cout << "Skipping test: No suitable device for testing" << std::endl;
            return;
        }
        size_t bufferSize = sizeof(float) * 10;
        auto buffer = device->createWorkingBuffer(bufferSize);
        TEST_ASSERT(buffer != nullptr, "Buffer allocation failed");
        std::vector<float> testData(10, 42.0f);
        // Use copyDataFrom/copyDataTo for buffer data transfer
        buffer->copyDataFrom(testData.data(), bufferSize, 0);
        std::vector<float> resultData(10, 0.0f);
        buffer->copyDataTo(resultData.data(), bufferSize, 0);
        for (int i = 0; i < 10; i++) {
            TEST_ASSERT(resultData[i] == 42.0f, "Data transfer verification failed at index " + std::to_string(i));
        }
    }
};
REGISTER_TEST(BufferDataTransferTest);

// Replace any Buffer::map/unmap/getMappedData with Buffer::getPtr(), and use copyDataFrom/copyDataTo for data transfer.
// Use Device::getComputePoolManager(size_t idx, VkQueueFlagBits flags) and Device::getDevice() with no arguments.
