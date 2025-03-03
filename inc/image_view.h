#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>


namespace runtime {

    class Image;

    class ImageView {
        public:
            ImageView (Image& image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags);


        private:
            VkImageView m_imageView;
            VkImageViewType m_viewType;
            VkFormat m_format;
            VkImageAspectFlags m_aspectFlags;
    }
}  // namespace runtime