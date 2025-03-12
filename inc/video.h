#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "error_handling.h"

namespace runtime {

class Device;

// Forward declarations
class VideoFrame;
class VideoDecoder;
class VideoEncoder;

/**
 * @brief Represents a video frame in GPU memory
 */
class VideoFrame {
public:
    static std::shared_ptr<VideoFrame> create(Device& device, uint32_t width, uint32_t height, VkFormat format);
    ~VideoFrame();

    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    VkFormat getFormat() const { return m_format; }
    VkImage getImage() const { return m_image; }
    VkImageView getImageView() const { return m_imageView; }

private:
    VideoFrame(Device& device, uint32_t width, uint32_t height, VkFormat format);
    void initialize();
    void cleanup();

    Device& m_device;
    uint32_t m_width{0};
    uint32_t m_height{0};
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkImage m_image{VK_NULL_HANDLE};
    VkDeviceMemory m_memory{VK_NULL_HANDLE};
    VkImageView m_imageView{VK_NULL_HANDLE};
};

/**
 * @brief Handles video decode operations using Vulkan Video extensions
 */
class VideoDecoder {
public:
    static std::shared_ptr<VideoDecoder> create(Device& device, VkFormat format);
    ~VideoDecoder();

    std::shared_ptr<VideoFrame> decodeFrame(const std::vector<uint8_t>& encodedData);

private:
    VideoDecoder(Device& device, VkFormat format);
    void initialize();
    void cleanup();

    Device& m_device;
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkVideoSessionKHR m_videoSession{VK_NULL_HANDLE};
    VkVideoSessionParametersKHR m_videoSessionParameters{VK_NULL_HANDLE};
};

/**
 * @brief Handles video encode operations using Vulkan Video extensions
 */
class VideoEncoder {
public:
    static std::shared_ptr<VideoEncoder> create(Device& device, VkFormat format);
    ~VideoEncoder();

    std::vector<uint8_t> encodeFrame(const std::shared_ptr<VideoFrame>& frame);

private:
    VideoEncoder(Device& device, VkFormat format);
    void initialize();
    void cleanup();

    Device& m_device;
    VkFormat m_format{VK_FORMAT_UNDEFINED};
    VkVideoSessionKHR m_videoSession{VK_NULL_HANDLE};
    VkVideoSessionParametersKHR m_videoSessionParameters{VK_NULL_HANDLE};
};

} // namespace runtime
