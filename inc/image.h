#ifndef IMAGE_H
#define IMAGE_H

#include "image_allocator.h"
#include "image_view.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <unordered_map>

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
            VmaAllocation getAllocation() const { return m_allocation; }

            // Returns a cached or creates a new image view
            std::shared_ptr<ImageView> createImageView(
                VkImageViewType viewType, 
                VkFormat format = VK_FORMAT_UNDEFINED, 
                VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

            // Transition image layout with automatic barrier insertion
            void transitionLayout(VkCommandBuffer cmdBuffer, 
                                 VkImageLayout newLayout,
                                 VkPipelineStageFlags srcStageMask,
                                 VkPipelineStageFlags dstStageMask,
                                 VkAccessFlags srcAccessMask,
                                 VkAccessFlags dstAccessMask);

        private:
            Image(std::shared_ptr<ImageAllocator> allocator, const VkImageCreateInfo& imageInfo);
            
            void initialize();
            
            // Cache for image views to avoid duplicate creation
            using ViewKey = std::tuple<VkImageViewType, VkFormat, VkImageAspectFlags>;
            struct ViewKeyHash {
                std::size_t operator()(const ViewKey& k) const;
            };
            std::unordered_map<ViewKey, std::weak_ptr<ImageView>, ViewKeyHash> m_viewCache;
            
            std::shared_ptr<ImageAllocator> m_allocator;
            VkImage m_image{VK_NULL_HANDLE};
            VmaAllocation m_allocation{VK_NULL_HANDLE};
            VkFormat m_format;
            VkExtent3D m_extent;
            VkImageLayout m_layout;
            VkImageUsageFlags m_usage;
    };

}  // namespace runtime

#endif // IMAGE_H
