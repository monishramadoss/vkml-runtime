// Include volk.h first, before any other Vulkan headers

#include "app.h"
#include "version.h"
#include "config.h"
#include "error_handling.h"
#include "debug_utils.h"
#include "device.h"

#include <vector>
#include <memory>


static bool enableValidationLayers = true;

namespace runtime {
    App::App() {
        initialize();
        m_debugMessenger = std::make_unique<DebugMessenger>();
        auto createInfo = DebugMessenger::getCreateInfo();
        m_debugMessenger->create(m_instance, &createInfo , nullptr, &m_debugMessenger->m_debugMessenger);
    }

    App::~App() {
        if (m_debugMessenger) {
            DebugMessenger::destroy(m_instance, m_debugMessenger->m_debugMessenger, nullptr);
        }
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }
    
    void App::initialize(){
   
        VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = Version::APPLICATION_NAME;
        appInfo.applicationVersion = Version::APPLICATION_VERSION;
        appInfo.pEngineName = Version::ENGINE_NAME;
        appInfo.engineVersion = Version::ENGINE_VERSION;
        appInfo.apiVersion = Version::API_VERSION;

        VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &appInfo;

        // Standard validation layers for Vulkan
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            debugCreateInfo = DebugMessenger::getCreateInfo();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
        // Load Vulkan instance functions after instance creation
        
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
        std::vector<VkPhysicalDevice> physical_devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, physical_devices.data());

        for (auto& pd : physical_devices) {
            m_devices.push_back(Device::create(m_instance, pd));
        }
    }

    
    void App::setupDebugMessenger() {    
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo = DebugMessenger::getCreateInfo();
        VK_CHECK(DebugMessenger::create(m_instance, &createInfo, nullptr, &m_debugMessenger->m_debugMessenger));
    }

}