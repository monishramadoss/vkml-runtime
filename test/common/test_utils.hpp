#pragma once

// Include order matters to avoid symbol conflicts
// First include the project headers, which will include volk.h
#include "runtime.h"
#include "device.h"
#include "queue.h"

#include <iostream>
#include <memory>
#include <cassert>

namespace runtime {
namespace test {

// Simple assertion macro that reports failures to stdout
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERTION FAILED: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

// A simple test fixture that initializes Vulkan instance and resources
class VulkanTestFixture {
public:
    VulkanTestFixture() : runtime_(nullptr) {}
    
    virtual ~VulkanTestFixture() {
        tearDown();
    }
    
    bool setUp() {
        runtime_ = std::make_unique<runtime::Runtime>();
        // Initialize the runtime (this is public in the real implementation but not exposed)
        // We'll just create a dummy instance for testing
        return true;
    }
    
    void tearDown() {
        runtime_.reset();
    }
    
    std::unique_ptr<runtime::Runtime> runtime_;
};

// Helper functions for creating test resources
inline VkBufferCreateInfo createDefaultBufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    return bufferInfo;
}

// Helper function to run a test and report results
inline bool runTest(bool (*testFunc)(), const char* testName) {
    std::cout << "Running test: " << testName << " ... ";
    bool result = testFunc();
    if (result) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
    return result;
}

} // namespace test
} // namespace runtime
