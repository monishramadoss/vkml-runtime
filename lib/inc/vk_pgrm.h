#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>




namespace vkrt {

	class work {
		VkSubmitInfo m_submit_info;
		VkCommandBuffer m_cmd_buffer;
		VkCommandPool m_cmd_pool;
	};

	class queue {
		VkQueueFamilyProperties m_properties;
		std::vector<work> m_work;
		std::vector<VkQueue> m_queues;
		uint32_t m_idx;

	public:
		queue(uint32_t idx, VkDevice device, VkQueueFamilyProperties& properties) : m_properties(properties), m_idx(idx){
			for (uint32_t i = 0; i < properties.queueCount; ++i) {
				VkQueue queue;
				vkGetDeviceQueue(device, idx, i, &queue);	
				m_queues.push_back(queue);
			}
		}

		bool isGraphicFamily() const { return m_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT; }
		bool isComputeFamily() const { return m_properties.queueFlags & VK_QUEUE_COMPUTE_BIT; }
		bool isTransferFamily() const { return m_properties.queueFlags & VK_QUEUE_TRANSFER_BIT; }
		bool isSparseBindingFamily() const { return m_properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT; }
		bool isVideoDecodeFamily() const { return m_properties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR; }
		bool isVideoEncodeFamily() const { return m_properties.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR; }
		bool isOtherFamily() const {
			return ~(VK_QUEUE_TRANSFER_BIT & VK_QUEUE_COMPUTE_BIT & VK_QUEUE_GRAPHICS_BIT &
				VK_QUEUE_SPARSE_BINDING_BIT & VK_QUEUE_VIDEO_DECODE_BIT_KHR &
				VK_QUEUE_VIDEO_ENCODE_BIT_KHR) & m_properties.queueFlags;
		}

	};



} // namespace vkrt