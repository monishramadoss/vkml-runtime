#pragma once

#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#ifdef DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif


#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <memory>
#include <vector>
#include <map>

#include "vk_pgrm.h"

void print_statistic(VmaStatistics stats) 
{
	printf("\t\t blockCount: %i \n", stats.blockCount);
	printf("\t\t blockBytes: %lli \n", stats.blockBytes);
	printf("\t\t allocationCount: %i \n", stats.allocationCount);
	printf("\t\t allocationBytes: %lli \n", stats.allocationBytes);
}

void print_statistic(VmaDetailedStatistics stats)
{
	printf("\t unusedRangeCount: %i \n", stats.unusedRangeCount);
	printf("\t unusedRangeSizeMin: %lli \n", stats.unusedRangeSizeMin);
	printf("\t unusedRangeSizeMax %lli \n", stats.unusedRangeSizeMin);
	printf("\t allocationSizeMin: %lli \n", stats.allocationSizeMin);
	printf("\t allocationSizeMax: %lli \n", stats.allocationSizeMax);
	print_statistic(stats.statistics);
}

void print_statistic(VmaTotalStatistics stats) {
	for (int i = 0; i < VK_MAX_MEMORY_TYPES; ++i)
	{
		printf("\t memoryType [%i]\n", i);
		print_statistic(stats.memoryType[i]);
	}
	for (int i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
	{
		printf("\t memoryHeap [%i]\n", i);
		print_statistic(stats.memoryHeap[i]);
	}
	printf("\t total\n");
	print_statistic(stats.total);
}

namespace vkrt {
	
	class Buffer;
	class Device {

		VkDevice m_dev;
		VkPipelineCache m_pipeline_cache;
		VkPhysicalDevice m_pDev;
		
		VmaAllocator m_allocator;
		std::vector<queue> m_queues;
		std::map<VkQueueFlags, queue> m_queue_map;
		std::map<uint32_t, std::pair<VkBuffer, VmaAllocation>> m_buffers;

		void setupQueues(std::vector< VkQueueFamilyProperties >& queueFamilies) {
			for (uint32_t i = 0; i < queueFamilies.size(); ++i)
			{
                m_queue_map.emplace(queueFamilies[i].queueFlags, queue(i, m_dev, queueFamilies[i]));
			}
		}

		size_t findQueueFamilies(std::vector<VkQueueFamilyProperties>& queueFamilies, std::vector < VkDeviceQueueCreateInfo>& queueCreateInfos)
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, nullptr);
			queueFamilies.resize(queueFamilyCount);
			queueCreateInfos.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, queueFamilies.data());

			int i = 0, j = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = i;
				queueCreateInfo.queueCount = queueFamily.queueCount;
				std::vector<float> queuePriorities(queueFamily.queueCount, 1.0f);
				queueCreateInfo.pQueuePriorities = queuePriorities.data();
				queueCreateInfos[i++] = queueCreateInfo;
				j += queueFamily.queueCount;
			}
			return j;
		}

	
		

		struct features {
			VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
			VkPhysicalDeviceVulkan11Features features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
			VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
			VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
			VkPhysicalDeviceCooperativeMatrixFeaturesKHR coo_matrix_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR };
		} m_feats;

		struct properties {
			VkPhysicalDeviceProperties2 device_properties_2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			VkPhysicalDeviceSubgroupProperties subgroup_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };
			VkPhysicalDeviceVulkan11Properties device_vulkan11_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
			VkPhysicalDeviceVulkan12Properties device_vulkan12_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
			VkPhysicalDeviceVulkan13Properties device_vulkan13_properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES };
		} m_props;
		
		void getFeaturesAndProperties(VkPhysicalDevice& pd) {		
			m_feats.features2.pNext = &m_feats.features11;
			m_feats.features11.pNext = &m_feats.features12;
			m_feats.features12.pNext = &m_feats.features13;
			m_feats.features13.pNext = &m_feats.coo_matrix_features;
			vkGetPhysicalDeviceFeatures2(pd, &m_feats.features2);
			
			m_props.device_properties_2.pNext = &m_props.device_vulkan11_properties;
			m_props.device_vulkan11_properties.pNext = &m_props.device_vulkan12_properties;
			m_props.device_vulkan12_properties.pNext = &m_props.device_vulkan13_properties;
			m_props.device_vulkan13_properties.pNext = &m_props.subgroup_properties;
			vkGetPhysicalDeviceProperties2(pd, &m_props.device_properties_2);
		}
	

		void allocate_buffer(const VkBufferCreateInfo* bufferCreateInfo, VmaAllocationCreateInfo* vmaAllocCreateInfo, VkBuffer* buffer, VmaAllocation* allocation, VmaAllocationInfo* alloc_info, uint32_t* buffer_id) {
			uint32_t memTypeIdx = 0;
			if (vmaFindMemoryTypeIndexForBufferInfo(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, &memTypeIdx) == VK_SUCCESS) {
				vmaAllocCreateInfo->memoryTypeBits = 1u << memTypeIdx;
				auto res = vmaCreateBuffer(m_allocator, bufferCreateInfo, vmaAllocCreateInfo, buffer, allocation, alloc_info);
				*buffer_id = m_buffers.size();
				m_buffers.emplace(*buffer_id, std::make_pair(*buffer, *allocation));
				CHECK_RESULT(res, "failure to allocate buffer");
			}
			else {
				printf("Failed to find memory type index\n");
			}
		}

		void destroy_buffer(VkBuffer buffer, VmaAllocation allocation) {
			vmaDestroyBuffer(m_allocator, buffer, allocation);
		}

		friend class Buffer;
	public:
		
		Device(const Device& other) {
			m_dev = other.m_dev;
			m_pipeline_cache = other.m_pipeline_cache;
			m_pDev = other.m_pDev;
			m_allocator = other.m_allocator;
			m_queues = other.m_queues;
			m_queue_map = other.m_queue_map;
			m_buffers = other.m_buffers;
			m_feats = other.m_feats;
			m_props = other.m_props;

		}

		Device& operator=(const Device& other) {
			m_dev = other.m_dev;
			m_pipeline_cache = other.m_pipeline_cache;
			m_pDev = other.m_pDev;
			m_allocator = other.m_allocator;
			m_queues = other.m_queues;
			m_queue_map = other.m_queue_map;
			m_buffers = other.m_buffers;
			m_feats = other.m_feats;
			m_props = other.m_props;

			return *this;
		}
				
		Device(VkInstance& inst, VkPhysicalDevice& pd) : m_pDev(pd)
		{			
			getFeaturesAndProperties(pd);
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_pDev, &queueFamilyCount, queueFamilies.data());

			std::vector < VkDeviceQueueCreateInfo> queueCreateInfos;
			size_t max_queues = findQueueFamilies(queueFamilies, queueCreateInfos);
			printf("Device has %zi queue families\n", max_queues);

			std::vector<const char*> deviceExtensions;
			if (m_feats.coo_matrix_features.cooperativeMatrix) {
				deviceExtensions.push_back(VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME);
			}


			VkDeviceCreateInfo deviceInfo = {};
			deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
			deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceInfo.enabledExtensionCount = deviceExtensions.size();
			deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

			vkCreateDevice(pd, &deviceInfo, nullptr, &m_dev);
			volkLoadDevice(m_dev);
			
			setupQueues(queueFamilies);

			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			vkCreatePipelineCache(m_dev, &pipelineCacheCreateInfo, nullptr, &m_pipeline_cache);

			VolkDeviceTable dt;
			volkLoadDeviceTable(&dt, m_dev);
						
			VmaVulkanFunctions vmaFuncs = {};
			vmaFuncs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
			vmaFuncs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
			vmaFuncs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
			vmaFuncs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
			vmaFuncs.vkAllocateMemory = dt.vkAllocateMemory;
			vmaFuncs.vkFreeMemory = dt.vkFreeMemory;
			vmaFuncs.vkMapMemory = dt.vkMapMemory;
			vmaFuncs.vkUnmapMemory = dt.vkUnmapMemory;
			vmaFuncs.vkFlushMappedMemoryRanges = dt.vkFlushMappedMemoryRanges;
			vmaFuncs.vkInvalidateMappedMemoryRanges = dt.vkInvalidateMappedMemoryRanges;
			vmaFuncs.vkBindBufferMemory = dt.vkBindBufferMemory;
			vmaFuncs.vkBindImageMemory = dt.vkBindImageMemory;
			vmaFuncs.vkGetBufferMemoryRequirements = dt.vkGetBufferMemoryRequirements;
			vmaFuncs.vkGetImageMemoryRequirements = dt.vkGetImageMemoryRequirements;
			vmaFuncs.vkCreateBuffer = dt.vkCreateBuffer;
			vmaFuncs.vkDestroyBuffer = dt.vkDestroyBuffer;
			vmaFuncs.vkCreateImage = dt.vkCreateImage;
			vmaFuncs.vkDestroyImage = dt.vkDestroyImage;
			vmaFuncs.vkCmdCopyBuffer = dt.vkCmdCopyBuffer;

			VmaAllocatorCreateInfo allocatorInfo = {};
			allocatorInfo.physicalDevice = pd;
			allocatorInfo.device = m_dev;
			allocatorInfo.pVulkanFunctions = &vmaFuncs;
			allocatorInfo.instance = inst;		
			allocatorInfo.preferredLargeHeapBlockSize = static_cast<VkDeviceSize>(1) << 28;
			allocatorInfo.vulkanApiVersion = volkGetInstanceVersion();
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;;
			CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator), "Failed to create allocator");
		}

		void destory() {
			for (auto& id_buf : m_buffers)
				destroy_buffer(id_buf.second.first, id_buf.second.second);

			vmaDestroyAllocator(m_allocator);
			vkDestroyPipelineCache(m_dev, m_pipeline_cache, nullptr);
			vkDestroyDevice(m_dev, nullptr);
		}

	};

	class Buffer
	{
		VkBuffer m_buffer;
		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocation_info;

		VkBufferCreateInfo m_buffer_create_info;
		VmaAllocationCreateInfo m_allocation_create_info;

		uint32_t m_buffer_id = UINT32_MAX;
	public:
		Buffer( Device& dev, size_t size, int flags)
		{
			m_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			m_buffer_create_info.size = size;
			m_buffer_create_info.usage = flags;
			m_buffer_create_info.pNext = nullptr;
			m_allocation_create_info.flags = 0;
			m_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

			VkBufferUsageFlagBits usage = static_cast<VkBufferUsageFlagBits>(flags);
			
			if (flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
				m_allocation_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			if (flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
				m_allocation_create_info.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			if (flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
				m_allocation_create_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			if(flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
				m_allocation_create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			//m_allocation_create_info.priority = 1.0f;
			m_allocation_create_info.pool = nullptr;
			m_allocation_create_info.requiredFlags = 0;
			m_allocation_create_info.preferredFlags = 0;
			m_allocation_create_info.memoryTypeBits = UINT32_MAX;
			dev.allocate_buffer(&m_buffer_create_info, &m_allocation_create_info, &m_buffer, &m_allocation, &m_allocation_info, &m_buffer_id);
		}

		Buffer& operator=(const Buffer& other) {
			m_buffer = other.m_buffer;
			m_allocation = other.m_allocation;
			m_allocation_info = other.m_allocation_info;
			m_buffer_create_info = other.m_buffer_create_info;
			m_allocation_create_info = other.m_allocation_create_info;
			m_buffer_id = other.m_buffer_id;
			return *this;
		}

		

		Buffer(const Buffer& other) {
			m_buffer = other.m_buffer;
			m_allocation = other.m_allocation;
			m_allocation_info = other.m_allocation_info;
			m_buffer_create_info = other.m_buffer_create_info;
			m_allocation_create_info = other.m_allocation_create_info;
			m_buffer_id = other.m_buffer_id;
		}


		void* data() {
			return m_allocation_info.pMappedData;
		}
		bool isLive(void) const
		{
			return m_buffer_id == UINT32_MAX;
		}
	};
};