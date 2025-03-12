#include "device.h"
#include "error_handling.h"
#include "compute_program.h"
#include "memory_manager.h"
#include "queue_manager.h"

#include "device_features.h"
#include "buffer_allocator.h"
#include "queue_manager.h"
#include "buffer.h"
#include "descriptor_manager.h"
#include "compute_packet.h"

namespace runtime
{
    static std::shared_ptr<Device> Device::create(VkInstance& instance, VkPhysicalDevice& pd)
    {
        return std::make_shared<Device>(instance, pd);
    }

    Device::Device(VkInstance& instance, VkPhysicalDevice& pd)
    {
        initialize(instance, pd);
    }

    Device::~Device() {
        cleanup();
    }

    Device::Device(Device&& other) noexcept 
        : m_device(other.m_device)
        , m_pipeline_cache(other.m_pipeline_cache)
        , m_physical_device(other.m_physical_device)
        , m_allocator(other.m_allocator)
        , m_features(std::move(other.m_features))
        , m_buffer_allocator(std::move(other.m_buffer_allocator))
        , m_memory_manager(std::move(other.m_memory_manager))
        , m_queue_manager(std::move(other.m_queue_manager))
    {
        other.m_device = VK_NULL_HANDLE;
        other.m_pipeline_cache = VK_NULL_HANDLE;
        other.m_physical_device = VK_NULL_HANDLE;
        other.m_allocator = VK_NULL_HANDLE;
    }

    Device& Device::operator=(Device&& other) noexcept {
        if (this != &other) {
            cleanup();
            
            m_device = other.m_device;
            m_pipeline_cache = other.m_pipeline_cache;
            m_physical_device = other.m_physical_device;
            m_allocator = other.m_allocator;
            m_features = std::move(other.m_features);
            m_buffer_allocator = std::move(other.m_buffer_allocator);
            m_memory_manager = std::move(other.m_memory_manager);
            m_queue_manager = std::move(other.m_queue_manager);

            other.m_device = VK_NULL_HANDLE;
            other.m_pipeline_cache = VK_NULL_HANDLE;
            other.m_physical_device = VK_NULL_HANDLE;
            other.m_allocator = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Device::initialize(VkInstance& instance, VkPhysicalDevice& pd)
    {
        m_physical_device = pd;
        
        // Create features first to use during device creation
        m_features = std::make_unique<DeviceFeatures>(pd);
        
        // Create queue manager to get queue create infos
        m_queue_manager = std::make_shared<QueueManager>(pd);
        
        // Create device with queue create infos
        VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        auto queueCreateInfos = m_queue_manager->getQueueCreateInfos();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        // Setup features chain
        const auto& features = m_features->getFeaturesChain();
        createInfo.pNext = &features;

        // Create device
        VkResult result = vkCreateDevice(pd, &createInfo, nullptr, &m_device);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        
        // Complete queue manager initialization with the device
        m_queue_manager->initialize(m_device);

        // Create pipeline cache
        VkPipelineCacheCreateInfo pipelineCacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        result = vkCreatePipelineCache(m_device, &pipelineCacheInfo, nullptr, &m_pipeline_cache);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline cache");
        }

        // Create memory allocator
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = pd;
        allocatorInfo.device = m_device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
        
        result = vmaCreateAllocator(&allocatorInfo, &m_allocator);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator");
        }
        
        // Create memory manager
        m_memory_manager = MemoryManager::create(m_allocator, m_device);
        
        // Create buffer allocator
        m_buffer_allocator = BufferAllocator::create(m_allocator, m_device);

        // Create descriptor manager
        m_descriptor_manager = std::make_unique<DescriptorManager>(m_device);
    }

    uint32_t Device::getComputeQueueFamilyIndex() const {
        if (!m_queue_manager) {
            throw std::runtime_error("Queue manager not initialized");
        }
        // Get first queue family that supports compute
        auto queueFamilies = m_queue_manager->findQueueFamilies();
        if (queueFamilies.empty()) {
            throw std::runtime_error("No compute queue families found");
        }
        return queueFamilies[0];
    }
    
    uint32_t Device::getTransferQueueFamilyIndex() const { 
        if (!m_queue_manager) {
            throw std::runtime_error("Queue manager not initialized");
        }
        // Get first queue family that supports transfer
        auto queueFamilies = m_queue_manager->findQueueFamilies();
        if (queueFamilies.empty()) {
            throw std::runtime_error("No transfer queue families found");
        }
        return queueFamilies[0];
    }
    VkQueue Device::getComputeQueue() const {
        if (!m_queue_manager) {
            throw std::runtime_error("Queue manager not initialized");
        }
        
        // Get first compute queue
        uint32_t queueFamilyIndex = getComputeQueueFamilyIndex();
        auto* dispatcher = m_queue_manager->getQueueDispatcher(queueFamilyIndex);
        if (!dispatcher) {
            throw std::runtime_error("No queue dispatcher found for compute queue family");
        }
        
        return dispatcher->getQueue();
    }

    VkQueue Device::getTransferQueue() const {
        if(!m_queue_manager) {
            throw std::runtime_error("Queue manager not initialized");
        }

        // Get first transfer queue

    }

    void Device::cleanup() {
        if (m_device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_device);
            
            m_buffer_allocator.reset();
            m_memory_manager.reset();
            m_queue_manager.reset();
            
            if (m_allocator != VK_NULL_HANDLE) {
                vmaDestroyAllocator(m_allocator);
                m_allocator = VK_NULL_HANDLE;
            }
            
            if (m_pipeline_cache != VK_NULL_HANDLE) {
                vkDestroyPipelineCache(m_device, m_pipeline_cache, nullptr);
                m_pipeline_cache = VK_NULL_HANDLE;
            }
            
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }

        if (m_descriptor_manager) {
            m_descriptor_manager->destroy();
            m_descriptor_manager.reset();
        }
    }

    std::shared_ptr<Buffer> Device::createStorageBuffer(uint32_t bufferInfo, bool isHostVisible) {
        return Buffer::createStorageBuffer(m_buffer_allocator, bufferInfo, isHostVisible);
    }

    std::shared_ptr<ComputeProgram> Device::createComputeProgram(const std::vector<uint32_t>& code) {
        if (!m_memory_manager) {
            throw std::runtime_error("Memory manager not initialized");
        }
        
        ComputeProgram::ProgramCreateInfo info {};
        info.code = code;
        info.entryPoint = "main";
        info.localSizeX = 1;
        info.localSizeY = 1;
        info.localSizeZ = 1;
        info.enablePushConstants = false;
        info.pushConstantSize = 0;

        return std::make_shared<ComputeProgram>(m_device, info);
    }

    void Device::updateDescriptorSets(uint32_t set_id, 
                                     std::shared_ptr<vkrt_compute_program> program,
                                     const std::vector<std::shared_ptr<vkrt_buffer>>& buffers) {
        // Implement descriptor set updates - this would typically be handled by a descriptor manager
        // or as part of the compute program's functionality
    }

    void Device::mapBuffer(VmaAllocation alloc, void** data) const {
        m_memory_manager->mapBuffer(alloc, data);
    }

    void Device::unmapBuffer(VmaAllocation alloc) const {
        m_memory_manager->unmapMemory(alloc);
    }

    VkResult Device::copyMemoryToBuffer(VmaAllocation dst, const void* src, size_t size, size_t dst_offset) const {
        m_memory_manager->copyToMemory(dst, src, size, dst_offset);
    }

    VkResult Device::copyBufferToMemory(VmaAllocation src, void* dst, size_t size, size_t src_offset) const {
        m_memory_manager->copyFromMemory(src, dst, size, src_offset);
    }

    void Device::createEvent(const VkEventCreateInfo* info, VkEvent* event) const {
        VkResult result = vkCreateEvent(m_device, info, nullptr, event);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create event");
        }
    }

    void Device::destroyEvent(VkEvent event) const {
        vkDestroyEvent(m_device, event, nullptr);
    }

    void Device::destroy() {
        cleanup();
    }

    std::shared_ptr<ComputePacket> Device::construct(const std::vector<uint32_t>& code) {
        // Forward to memory manager to create the compute program
        if (!m_memory_manager) {
            throw std::runtime_error("Memory manager not initialized");
        }
        
        return createComputeProgram(code);
    }

    void Device::update(uint32_t set_id, 
                      std::shared_ptr<vkrt_compute_program> program,
                      const std::vector<std::shared_ptr<vkrt_buffer>>& buffers) {
        // Delegate to descriptor manager to handle descriptor updates
        if (!m_descriptor_manager) {
            throw std::runtime_error("Descriptor manager not initialized");
        }
        m_descriptor_manager->updateBufferDescriptors(set_id, program, buffers);
    }

    // Fix enum definition to use proper C++ enum class
    enum class BlobField {
        NAME_SIZE = 0,
        NAME_START,
        // Dynamic offset based on name size
        ARGS_SIZE,
        ARGS_START,
        // Rest are payload values
    };

    void Device::submitBlobs(const std::vector<uint32_t>& blobs) {
        if (blobs.empty()) {
            return;
        }

        // Parse the blob structure
        uint32_t name_size = blobs[static_cast<size_t>(BlobField::NAME_SIZE)];
        if (blobs.size() < name_size + 2) { // Ensure we have NAME_SIZE + name + ARGS_SIZE
            throw std::runtime_error("Invalid blob format: too small");
        }

        // Extract operation name
        std::string name;
        for (size_t i = 0; i < name_size; ++i) {
            // Properly unpack characters from uint32_t
            uint32_t value = blobs[static_cast<size_t>(BlobField::NAME_START) + i];
            name.append(reinterpret_cast<const char*>(&value), sizeof(uint32_t));
        }

        // Get arguments size and offset
        uint32_t args_size_offset = static_cast<size_t>(BlobField::NAME_START) + name_size;
        uint32_t args_size = blobs[args_size_offset];
        uint32_t args_start = args_size_offset + 1;

        if (blobs.size() < args_start + args_size) {
            throw std::runtime_error("Invalid blob format: incomplete arguments");
        }

        // Extract arguments
        std::vector<uint32_t> args(blobs.begin() + args_start, 
                                  blobs.begin() + args_start + args_size);

        // Dispatch to appropriate handler based on operation name
        dispatchOperation(name, args);

    }

private:
    void dispatchOperation(const std::string& name, const std::vector<uint32_t>& args) {
        // Map operation names to handlers
        static const std::unordered_map<std::string, 
            std::function<void(Device&, const std::vector<uint32_t>&)>> handlers = {
            {"compute", [](Device& dev, const std::vector<uint32_t>& args) {
                // Handle compute operation
                dev.handleComputeOperation(args);
            }},
            {"transfer", [](Device& dev, const std::vector<uint32_t>& args) {
                // Handle transfer operation
                dev.handleTransferOperation(args);
            }},
            // Add more operation handlers as needed
        };

        auto it = handlers.find(name);
        if (it != handlers.end()) {
            it->second(*this, args);
        } else {
            throw std::runtime_error("Unknown operation: " + name);
        }
    }

    void handleComputeOperation(const std::vector<uint32_t>& args) {
        if (args.size() < 4) {
            throw std::runtime_error("Invalid compute operation arguments");
        }
        
        // Parse arguments
        uint32_t index = 0;
        
        // Get program code size and read SPIR-V code
        uint32_t codeSize = args[index++];
        if (index + codeSize > args.size()) {
            throw std::runtime_error("Invalid SPIR-V code size");
        }
        std::vector<uint32_t> spirvCode(args.begin() + index, args.begin() + index + codeSize);
        index += codeSize;
        
        // Create compute program from SPIR-V code
        std::shared_ptr<ComputeProgram> program = createComputeProgram(spirvCode);
        
        // Parse dispatch dimensions
        if (index + 3 > args.size()) {
            throw std::runtime_error("Missing dispatch dimensions");
        }
        uint32_t dispatchX = args[index++];
        uint32_t dispatchY = args[index++];
        uint32_t dispatchZ = args[index++];
        
        // Parse buffer information
        if (index >= args.size()) {
            throw std::runtime_error("Missing buffer count");
        }
        uint32_t bufferCount = args[index++];
        
        // Collect buffer references
        std::vector<std::shared_ptr<vkrt_buffer>> buffers;
        for (uint32_t i = 0; i < bufferCount; ++i) {
            if (index + 2 > args.size()) {
                throw std::runtime_error("Insufficient buffer information");
            }
            
            uint32_t bufferId = args[index++];
            uint32_t bufferSize = args[index++];
            
            // Create or retrieve buffer (assuming a simple buffer registry by ID)
            // In a real implementation, you'd likely have a buffer cache/registry
            auto buffer = createStorageBuffer(bufferSize, true);
            buffers.push_back(buffer);
        }
        
        // Update descriptors
        uint32_t descriptorSetId = 0; // Use default descriptor set
        update(descriptorSetId, program, buffers);
        
        // Execute compute work
        VkQueue computeQueue = getComputeQueue();
        uint32_t queueFamilyIndex = getComputeQueueFamilyIndex();
        
        // Create command buffer and record commands
        VkCommandPool commandPool = m_queue_manager->getCommandPool(queueFamilyIndex);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
    }

    void handleTransferOperation(const std::vector<uint32_t>& args) {
        if (args.size() < 6) {
            throw std::runtime_error("Invalid transfer operation arguments");
        }
        
        uint32_t index = 0;
        
        // Parse transfer type
        uint32_t transferType = args[index++];
        enum TransferType {
            BUFFER_TO_BUFFER = 0,
            HOST_TO_BUFFER = 1,
            BUFFER_TO_HOST = 2
        };
        
        // Parse source and destination information
        uint32_t srcId = args[index++];
        uint32_t dstId = args[index++];
        uint32_t size = args[index++];
        uint32_t srcOffset = args[index++];
        uint32_t dstOffset = args[index++];
        
        // Validate size
        if (size == 0) {
            throw std::runtime_error("Transfer size must be greater than zero");
        }
        
        // Handle based on transfer type
        switch (transferType) {
            case BUFFER_TO_BUFFER: {
                // Get source and destination buffers
                std::shared_ptr<Buffer> srcBuffer = findBuffer(srcId);
                std::shared_ptr<Buffer> dstBuffer = findBuffer(dstId);
                
                if (!srcBuffer || !dstBuffer) {
                    throw std::runtime_error("Invalid buffer IDs for transfer operation");
                }
                
                // Create and record command buffer for the transfer
                VkQueue transferQueue = getTransferQueue(); // Or get transfer queue if available
                uint32_t queueFamilyIndex = getTransferQueueFamilyIndex();
                VkCommandPool commandPool = m_queue_manager->getCommandPool(queueFamilyIndex);
                
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = commandPool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = 1;
                
                VkCommandBuffer commandBuffer;
                if (vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to allocate command buffer for transfer");
                }
                break;
            }
            case HOST_TO_BUFFER: {
                // Handle host to buffer transfer
                break;
            }
            case BUFFER_TO_HOST: {
                // Handle buffer to host transfer
                break;
            }
            default:
                throw std::runtime_error("Unknown transfer type");
        }
    }

} // namespace runtime