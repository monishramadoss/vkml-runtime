#ifndef VOLK_HH
#define VOLK_HH
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif // VOLK_HH

#include "version.h"
#include "error_handling.h"
#include "device.h"
#include "runtime.h"

#include "storage.h"

static bool enableValidationLayers = false;

namespace runtime {

    std::shared_ptr<Runtime> Runtime::create()
    {
        return std::make_shared<Runtime>();
    }

    Runtime::Runtime() {
        initialize();        
    }

    Runtime::~Runtime() {
        cleanup();
    }
    
    void Runtime::cleanup()
    {    
        for (auto &d : m_devices)
        {
            d.reset();
        }
        if (m_debugMessenger)
        {
            m_debugMessenger->destroy(m_instance, nullptr);
        }
        if (m_instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
            // volkFinalize();
        }
    }

    std::shared_ptr<Buffer> Runtime::createDeviceBuffer(size_t size, uint32_t device_id)
    {
        check_condition(device_id < m_devices.size(), "Device ID is out of range");
        return m_devices[device_id]->createWorkingBuffer(size, true);            
       
    }

    std::shared_ptr<Buffer> Runtime::createHostBuffer(size_t size, uint32_t device_id)
    {
        check_condition(device_id < m_devices.size(), "Device ID is out of range");
        return m_devices[device_id]->createWorkingBuffer(size, false);
    }

    void Runtime::copyData(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, size_t size)
    {
        src->copyDataTo(dst, size);
    }

    void Runtime::copyData(void *src, std::shared_ptr<Buffer> dst, size_t size, size_t dst_offset, size_t src_offset)
    {
        dst->copyDataFrom(src, size, dst_offset, src_offset, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    }

    void Runtime::copyData(std::shared_ptr<Buffer> src, void *dst, size_t size, size_t dst_offset, size_t src_offset)
    {
        src->copyDataTo(dst, size, dst_offset, src_offset, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    }

    std::shared_ptr<Program> Runtime::createProgram(std::shared_ptr<Device>& device, const std::vector<uint32_t> &shader_code){
        return device->createProgram(shader_code);
    }
   
    size_t Runtime::deviceCount() const
    {   
        return m_physicalDevices.size();   
    }

    void Runtime::initialize()
    {
        m_threadPool = ThreadPool::create();
        check_result(volkInitialize(), "Failed to initialize volk");
        VkApplicationInfo appInfo  {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = Version::APPLICATION_NAME;
        appInfo.applicationVersion = Version::APPLICATION_VERSION;
        appInfo.pEngineName = Version::ENGINE_NAME;
        appInfo.engineVersion = Version::ENGINE_VERSION;
        appInfo.apiVersion = Version::API_VERSION;
        appInfo.pNext = nullptr;
        std::vector<const char *> available_extensions_names;
        std::vector<const char*> available_layers_names;

        VkInstanceCreateInfo createInfo  {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.flags = 0;
        
        uint32_t extension_count = 0;
        check_result(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr),
                     "Failed to enumerate instance extensions");
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        check_result(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data()),
                     "Failed to enumerate instance extensions");
        for (const auto &extension : available_extensions)
            available_extensions_names.push_back(extension.extensionName);
          uint32_t layer_count = 0;
        check_result(vkEnumerateInstanceLayerProperties(&layer_count, nullptr), "Failed to enumerate instance layers");
        std::vector<VkLayerProperties> available_layers(layer_count);
        check_result(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()),
                     "Failed to enumerate instance layers");
        
        for (const auto &layer : available_layers) {
            // Skip performance layers based on configuration
#ifdef DISABLE_PERFORMANCE_COUNTERS
            // Filter out any performance-related layers that may cause issues
            if (strstr(layer.layerName, "performance") == nullptr && 
                strstr(layer.layerName, "perf") == nullptr &&
                strstr(layer.layerName, "metric") == nullptr) {
                available_layers_names.push_back(layer.layerName);
            } else {
                LOG_INFO("Skipping performance layer: %s", layer.layerName);
            }
#else
            available_layers_names.push_back(layer.layerName);
#endif
        }


        createInfo.enabledExtensionCount = available_extensions_names.size();
        createInfo.ppEnabledExtensionNames = available_extensions_names.data();
        createInfo.enabledLayerCount = available_layers_names.size();
        createInfo.ppEnabledLayerNames = available_layers_names.data();
        

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            debugCreateInfo = DebugMessenger::getCreateInfo();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;        
            std::vector<const char*> extensions = {};
            m_debugMessenger = DebugMessenger::create(m_instance, createInfo, extensions);
        } else {            
            auto res = vkCreateInstance(&createInfo, nullptr, &m_instance);
        }

        volkLoadInstance(m_instance);

        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
        m_physicalDevices.resize(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, m_physicalDevices.data());     
    }

  
    inline std::shared_ptr<DebugMessenger> DebugMessenger::create(VkInstance instance, VkInstanceCreateInfo &createInfo,
                                                                  std::vector<const char *> extensions)
    {
        return std::make_shared<DebugMessenger>(instance, createInfo, extensions);
    }

    inline DebugMessenger::DebugMessenger(VkInstance instance, VkInstanceCreateInfo &createInfo,
                                          std::vector<const char *> extensions)
    {
        initialize(instance, createInfo, extensions, &m_debugMessenger);
    }

    inline VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
    {
        // Replace std::cerr with logger
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_DEBUG("validation layer: %s", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO("validation layer: %s", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARNING("validation layer: %s", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR("validation layer: %s", pCallbackData->pMessage);
            break;
        default:
            break;
        }
        return VK_FALSE;
    }

    inline VkDebugUtilsMessengerCreateInfoEXT DebugMessenger::getCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;
        return debugCreateInfo;
    }

    // Add overload for instance member version
    inline VkResult DebugMessenger::_create(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        return func ? func(instance, pCreateInfo, pAllocator, &m_debugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Keep static version for backward compatibility
    inline VkResult DebugMessenger::_create(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator,
                                            VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        return func ? func(instance, pCreateInfo, pAllocator, pDebugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Add instance member version
    inline void DebugMessenger::destroy(VkInstance instance, const VkAllocationCallbacks *pAllocator)
    {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func && m_debugMessenger != VK_NULL_HANDLE)
        {
            func(instance, m_debugMessenger, pAllocator);
            m_debugMessenger = VK_NULL_HANDLE;
        }
    }

    inline void DebugMessenger::initialize(VkInstance instance, VkInstanceCreateInfo &createInfo,
                                           std::vector<const char *> &extensions,
                                           VkDebugUtilsMessengerEXT *debugMessenger)
    {
#ifdef _DEBUG
        if (checkValidationLayerSupport())
        {
            auto debugCreateInfo = DebugMessenger::getCreateInfo();

            // Update to use the array size instead of the vector size
            createInfo.enabledLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;

            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.pNext = &debugCreateInfo;

            if (instance == VK_NULL_HANDLE)
            {
                throw std::runtime_error("Invalid Vulkan instance handle");
            }

            if (_create(instance, &debugCreateInfo, nullptr, debugMessenger) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create debug messenger");
            }
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }
#endif
    }

    inline bool DebugMessenger::checkValidationLayerSupport()
    {
#ifdef _DEBUG
        uint32_t layerCount = 0;
        VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to enumerate instance layer properties: %d", result);
            return false;
        }

        std::vector<VkLayerProperties> availableLayers(layerCount);
        result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to enumerate instance layer properties: %d", result);
            return false;
        }

        // Update to use the array elements directly
        const size_t validationLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);

        for (size_t i = 0; i < validationLayerCount; i++)
        {
            const char *layerName = VALIDATION_LAYERS[i];
            bool layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                LOG_WARNING("Validation layer '%s' not found", layerName);
                return false;
            }
        }

        LOG_DEBUG("All requested validation layers are available");
        return true;
#else
        return false;
#endif
    }

    std::shared_ptr<Device> Runtime::getDevice(uint32_t device_id, const std::vector<uint32_t> &queue_counts) 
    {
        
        check_condition(device_id < m_physicalDevices.size(), "Device ID is out of range");    
        if (m_devices.size() <= device_id)
        {
            m_devices.push_back(Device::create(m_threadPool, m_instance, m_physicalDevices.at(device_id), queue_counts));
            check_condition(m_devices[device_id] != nullptr, "Failed to create device");
        }
        return m_devices[device_id];
    }


    } // namespace runtime