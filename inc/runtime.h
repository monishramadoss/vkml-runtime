#ifndef APP_H
#define APP_H

#include "config.h"

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

#include "logging.h" 

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

#include "queue.h"
#include "storage.h"
#include "device.h"
#include "program.h"

namespace runtime {
    // Forward declarations

    class DebugMessenger
    {
      public:
        static std::shared_ptr<DebugMessenger> create(VkInstance instance, VkInstanceCreateInfo &createInfo,
                                                      std::vector<const char *> extensions = {});

        DebugMessenger(VkInstance instance, VkInstanceCreateInfo &createInfo, std::vector<const char *> extensions);
        // Change from constexpr std::vector to inline static array
        inline static const char *VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};

        // Add member to store debug messenger handle
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                            void *pUserData);

        static VkDebugUtilsMessengerCreateInfoEXT getCreateInfo();

        // Add overload for instance member version
        VkResult _create(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                         const VkAllocationCallbacks *pAllocator);

        // Keep static version for backward compatibility
        static VkResult _create(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);

        // Add instance member version
        void destroy(VkInstance instance, const VkAllocationCallbacks *pAllocator = nullptr);
        static void initialize(VkInstance instance, VkInstanceCreateInfo &createInfo,
                               std::vector<const char *> &extensions, VkDebugUtilsMessengerEXT *debugMessenger);
        static bool checkValidationLayerSupport();
    };

    
    class Buffer;
    class Program;
    class Process;

    class Runtime {
    public:
        static std::shared_ptr<Runtime> create();

        Runtime();
        ~Runtime();

        // Prevent copying
        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;

        VkInstance getInstance() const { return m_instance; }
        std::shared_ptr<Buffer> createDeviceBuffer(size_t size, uint32_t device_id);
        std::shared_ptr<Buffer> createHostBuffer(size_t size, uint32_t device_id);
        void copyData(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, size_t size);
        void copyData(void *src, std::shared_ptr<Buffer> dst, size_t size, size_t dst_offset, size_t src_offset);
        void copyData(std::shared_ptr<Buffer> src, void *dst, size_t size, size_t dst_offset, size_t src_offset);
        std::shared_ptr<Program> createProgram(std::shared_ptr<Device> &device,
                                               const std::vector<uint32_t> &shader_code);
        size_t deviceCount() const;
        std::shared_ptr<Device> getDevice(uint32_t device_id, const std::vector<uint32_t>& queue_sizes) ;
        // Create a device
        std::vector<std::shared_ptr<Device>> pullDevices() const {return m_devices;}
    private:
        void initialize();
        void cleanup();

        VkInstance m_instance;
        std::shared_ptr<DebugMessenger> m_debugMessenger {nullptr};
        bool m_initialized{false};
        std::vector<std::shared_ptr<Device>> m_devices;
        std::vector<VkPhysicalDevice> m_physicalDevices;
        std::shared_ptr<ThreadPool> m_threadPool;
    };


 

} // namespace runtime

#endif // APP_H