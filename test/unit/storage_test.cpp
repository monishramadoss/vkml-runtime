#include <gtest/gtest.h>
#include "common/test_utils.hpp"
#include "storage.h"

namespace vkml {
namespace test {

class StorageTest : public VulkanTestFixture {
protected:
    void SetUp() override {
        VulkanTestFixture::SetUp();
        device_ = std::make_unique<Device>(runtime_->getInstance());
        ASSERT_TRUE(device_->initialize());
    }

    void TearDown() override {
        device_.reset();
        VulkanTestFixture::TearDown();
    }

    std::unique_ptr<Device> device_;
};

TEST_F(StorageTest, BufferCreation) {
    Storage storage(device_.get());
    
    VkBufferCreateInfo bufferInfo = createDefaultBufferCreateInfo(1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VkBuffer buffer;
    VkDeviceMemory memory;
    
    EXPECT_TRUE(storage.createBuffer(bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, buffer, memory));
    
    // Cleanup
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_->getHandle(), buffer, nullptr);
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_->getHandle(), memory, nullptr);
    }
}

TEST_F(StorageTest, BufferMapping) {
    Storage storage(device_.get());
    
    const size_t bufferSize = 1024;
    VkBufferCreateInfo bufferInfo = createDefaultBufferCreateInfo(bufferSize, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    
    VkBuffer buffer;
    VkDeviceMemory memory;
    
    ASSERT_TRUE(storage.createBuffer(bufferInfo, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        buffer, memory));
    
    // Test mapping
    void* data = nullptr;
    EXPECT_TRUE(storage.mapMemory(memory, 0, bufferSize, &data));
    ASSERT_NE(data, nullptr);
    
    // Write some test data
    uint32_t testData[4] = {1, 2, 3, 4};
    memcpy(data, testData, sizeof(testData));
    
    // Unmap
    storage.unmapMemory(memory);
    
    // Cleanup
    vkDestroyBuffer(device_->getHandle(), buffer, nullptr);
    vkFreeMemory(device_->getHandle(), memory, nullptr);
}

TEST_F(StorageTest, MemoryTypeSelection) {
    Storage storage(device_.get());
    
    VkBufferCreateInfo bufferInfo = createDefaultBufferCreateInfo(1024, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    
    VkBuffer buffer;
    ASSERT_EQ(vkCreateBuffer(device_->getHandle(), &bufferInfo, nullptr, &buffer), VK_SUCCESS);
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_->getHandle(), buffer, &memRequirements);
    
    uint32_t memoryTypeIndex;
    EXPECT_TRUE(storage.findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &memoryTypeIndex));
    
    // Cleanup
    vkDestroyBuffer(device_->getHandle(), buffer, nullptr);
}

} // namespace test
} // namespace vkml
