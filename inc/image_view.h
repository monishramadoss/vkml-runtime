#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>
#include "error_handling.h"

namespace runtime {

    class Image;

    class ImageView {
        public:
            ImageView(Image& image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags);
            ~ImageView();

            // Prevent copying
            ImageView(const ImageView&) = delete;
            ImageView& operator=(const ImageView&) = delete;

            // Allow moving
            ImageView(ImageView&& other) noexcept;
            ImageView& operator=(ImageView&& other) noexcept;

            VkImageView getImageView() const { return m_imageView; }
            VkImageViewType getViewType() const { return m_viewType; }
            VkFormat getFormat() const { return m_format; }
            VkImageAspectFlags getAspectFlags() const { return m_aspectFlags; }

        private:
            void cleanup();
            
            VkDevice m_device{VK_NULL_HANDLE};
            VkImageView m_imageView{VK_NULL_HANDLE};
            VkImageViewType m_viewType;
            VkFormat m_format;
            VkImageAspectFlags m_aspectFlags;
    };
}  // namespace runtime

#endif // IMAGE_VIEW_H