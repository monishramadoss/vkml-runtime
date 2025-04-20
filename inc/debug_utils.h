#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H
#include "config.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <volk.h>

#include "logging.h" // Include the logging header

namespace runtime {

class DebugMessenger {
public:
    static std::shared_ptr<DebugMessenger> create(VkInstance instance, VkInstanceCreateInfo& createInfo, std::vector<const char*> extensions={})
    {
        return std::make_shared<DebugMessenger>(instance, createInfo, extensions);
    }
    

    DebugMessenger(VkInstance instance, VkInstanceCreateInfo& createInfo, std::vector<const char*> extensions)
    {
        initialize(instance, createInfo, extensions, &m_debugMessenger);
    } 
    // Change from constexpr std::vector to inline static array
    inline static const char* VALIDATION_LAYERS[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    // Add member to store debug messenger handle
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        // Replace std::cerr with logger
        switch (messageSeverity) {
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

    static VkDebugUtilsMessengerCreateInfoEXT getCreateInfo() {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;
        return debugCreateInfo;
    }

    // Add overload for instance member version
    VkResult _create(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        return func ? func(instance, pCreateInfo, pAllocator, &m_debugMessenger)
                   : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Keep static version for backward compatibility
    static VkResult _create(
        VkInstance instance,
        VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        return func ? func(instance, pCreateInfo, pAllocator, pDebugMessenger)
                   : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Add instance member version
    void destroy(VkInstance instance, const VkAllocationCallbacks* pAllocator = nullptr) 
    {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func && m_debugMessenger != VK_NULL_HANDLE) {
            func(instance, m_debugMessenger, pAllocator);
            m_debugMessenger = VK_NULL_HANDLE;
        }
    }

    static void initialize(VkInstance instance, 
                                   VkInstanceCreateInfo& createInfo,
                                   std::vector<const char*>& extensions,
                                   VkDebugUtilsMessengerEXT* debugMessenger) {
#ifdef _DEBUG
        if (checkValidationLayerSupport()) {
            auto debugCreateInfo = DebugMessenger::getCreateInfo();
            
            // Update to use the array size instead of the vector size
            createInfo.enabledLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
            
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.pNext = &debugCreateInfo;

            if (_create(instance, &debugCreateInfo, nullptr, debugMessenger) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create debug messenger");
            }
        } else {
            createInfo.enabledLayerCount = 0;
        }
#endif
    }

    static bool checkValidationLayerSupport() {
#ifdef _DEBUG
        uint32_t layerCount = 0;
        VkResult result = VK_INCOMPLETE;// vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to enumerate instance layer properties: %d", result);
            return false;
        }

        std::vector<VkLayerProperties> availableLayers(layerCount);
        result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to enumerate instance layer properties: %d", result);
            return false;
        }

        // Update to use the array elements directly
        const size_t validationLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
        
        for (size_t i = 0; i < validationLayerCount; i++) {
            const char* layerName = VALIDATION_LAYERS[i];
            bool layerFound = false;
            
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            
            if (!layerFound) {
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
};

} // namespace runtime

#endif // DEBUG_UTILS_H