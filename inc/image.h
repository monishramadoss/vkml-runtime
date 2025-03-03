#pragma once
#include "image_allocator.h"
#include "image_view.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>



namespace runtime {

    class Image {
        public:
            static std::shared_ptr<Image> createImage(
                std::shared_ptr<ImageAllocator> allocator,
                const VkImageCreateInfo& imageInfo);

            ~Image();

            VkImage getImage() const { return m_image; }
            VkFormat getFormat() const { return m_format; }
            VkExtent3D getExtent() const { return m_extent; }
            VkImageLayout getLayout() const { return m_layout; }

            std::shared_ptr<ImageView> createImageView(VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags);

        private:
            Image(std::shared_ptr<ImageAllocator> allocator, const VkImageCreateInfo& imageInfo);

            void initialize();

            std::shared_ptr<ImageAllocator> m_allocator;
            VkImage m_image{VK_NULL_HANDLE};
            VkFormat m_format;
            VkExtent3D m_extent;
            VkImageLayout m_layout;
    };

}  // namespace runtime