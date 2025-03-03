#include "version.h"
#include "error_handling.h"
#include "debug_utils.h"



namespace runtime {
    App::App() {
        VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        appInfo.pApplicationName = Version::APPLICATION_NAME;
        appInfo.applicationVersion = Version::APPLICATION_VERSION;
        appInfo.pEngineName = Version::ENGINE_NAME;
        appInfo.engineVersion = Version::ENGINE_VERSION;
        appInfo.apiVersion = Version::API_VERSION;

        VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
        volkInitialize();
        volkLoadInstance(m_instance);
    }

    App::~App() {
        vkDestroyInstance(m_instance, nullptr);
    }
    
}