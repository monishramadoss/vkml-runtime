#include <gtest/gtest.h>
#include "common/test_utils.hpp"
#include "queue.h"

namespace vkml {
namespace test {

class QueueTest : public VulkanTestFixture {
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

TEST_F(QueueTest, QueueCreation) {
    Queue queue(device_.get());
    EXPECT_TRUE(queue.initialize());
}

TEST_F(QueueTest, ComputeQueueSupport) {
    Queue queue(device_.get());
    ASSERT_TRUE(queue.initialize());
    EXPECT_TRUE(queue.supportsCompute());
}

TEST_F(QueueTest, SubmitCommandBuffer) {
    Queue queue(device_.get());
    ASSERT_TRUE(queue.initialize());
    
    // Create and submit a simple command buffer
    VkCommandPool commandPool;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queue.getFamilyIndex();
    
    ASSERT_EQ(vkCreateCommandPool(device_->getHandle(), &poolInfo, nullptr, &commandPool), VK_SUCCESS);
    
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    ASSERT_EQ(vkAllocateCommandBuffers(device_->getHandle(), &allocInfo, &commandBuffer), VK_SUCCESS);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    ASSERT_EQ(vkBeginCommandBuffer(commandBuffer, &beginInfo), VK_SUCCESS);
    ASSERT_EQ(vkEndCommandBuffer(commandBuffer), VK_SUCCESS);
    
    EXPECT_TRUE(queue.submit(commandBuffer));
    EXPECT_TRUE(queue.waitIdle());
    
    vkFreeCommandBuffers(device_->getHandle(), commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device_->getHandle(), commandPool, nullptr);
}

} // namespace test
} // namespace vkml
