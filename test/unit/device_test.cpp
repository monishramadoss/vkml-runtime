#include "common/test_utils.hpp"
#include "device.h"
#include "device_features.h"

namespace vkml {
namespace test {

class DeviceTestFixture : public VulkanTestFixture {
public:
    bool runTest();
};

bool testDeviceCreation() {
    VulkanTestFixture fixture;
    if (!fixture.setUp()) {
        return false;
    }
    
    auto device = std::make_unique<Device>(fixture.runtime_->getInstance());
    TEST_ASSERT(device->initialize(), "Device initialization failed");
    return true;
}

bool testPhysicalDeviceEnumeration() {
    VulkanTestFixture fixture;
    if (!fixture.setUp()) {
        return false;
    }
    
    auto device = std::make_unique<Device>(fixture.runtime_->getInstance());
    auto physicalDevices = device->enumeratePhysicalDevices();
    TEST_ASSERT(physicalDevices.size() > 0, "No physical devices found");
    return true;
}

bool testFeatureSupport() {
    VulkanTestFixture fixture;
    if (!fixture.setUp()) {
        return false;
    }
    
    auto device = std::make_unique<Device>(fixture.runtime_->getInstance());
    TEST_ASSERT(device->initialize(), "Device initialization failed");
    
    DeviceFeatures features;
    TEST_ASSERT(device->getFeatures(features), "Failed to get device features");
    return true;
}

bool testQueueFamilyProperties() {
    VulkanTestFixture fixture;
    if (!fixture.setUp()) {
        return false;
    }
    
    auto device = std::make_unique<Device>(fixture.runtime_->getInstance());
    TEST_ASSERT(device->initialize(), "Device initialization failed");
    
    auto queueFamilies = device->getQueueFamilyProperties();
    TEST_ASSERT(queueFamilies.size() > 0, "No queue families found");
    return true;
}

} // namespace test
} // namespace vkml

// Main function that runs all tests
int main() {
    bool allTestsPassed = true;
    
    // Run all device tests
    allTestsPassed &= vkml::test::runTest(vkml::test::testDeviceCreation, "Device Creation");
    allTestsPassed &= vkml::test::runTest(vkml::test::testPhysicalDeviceEnumeration, "Physical Device Enumeration");
    allTestsPassed &= vkml::test::runTest(vkml::test::testFeatureSupport, "Feature Support");
    allTestsPassed &= vkml::test::runTest(vkml::test::testQueueFamilyProperties, "Queue Family Properties");
    
    // Return appropriate exit code
    return allTestsPassed ? 0 : 1;
}
