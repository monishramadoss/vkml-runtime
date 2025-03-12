#include "runtime.h"
#include "app.h"
#include "device.h"
#include "version.h"
#include <stdexcept>

namespace runtime
{
    Runtime::Runtime()       
    {
        initialize();
    }

    Runtime::~Runtime()
    {
        cleanup();
    }

    void Runtime::initialize()
    {
        // Create Vulkan instance
        // Application info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = Version::APPLICATION_NAME;
        appInfo.applicationVersion = Version::APPLICATION_VERSION;
        appInfo.pEngineName = Version::ENGINE_NAME;
        appInfo.engineVersion = Version::ENGINE_VERSION;
        appInfo.apiVersion = Version::API_VERSION;
        
        // Instance creation info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        
        // Add required extensions
        std::vector<const char*> extensions;
        // TODO: Add necessary extensions
        
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        
        // Create the instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance");
        }

        
        // Enumerate physical devices
        std::vector<VkPhysicalDevice> physicalDevices;
        enumerateDevices(physicalDevices);
        
        if (physicalDevices.empty()) {
            throw std::runtime_error("No Vulkan-capable devices found");
        }
        
        // Initialize devices
        for (auto i = 0; i < physicalDevices.size(); i++) {
            m_devices.push_back(app->createDevice(physicalDevices[i]));
        }

        m_running = true;
    }
    
   

    void Runtime::run(App& appInstance)
    {
        m_running = true;
        appInstance.m_running = true;
    }

    void Runtime::stop()
    {
        m_running = false;
    }

    void Runtime::enumerateDevices(std::vector<VkPhysicalDevice>& physicalDevices)
    {
        physicalDevices = app->enumeratePhysicalDevices();
    }
    
    void Runtime::cleanup()
    {
        // Clean up devices first
        for (auto& device : m_devices)
        {
            if (device) {
                device->cleanup();
            }
        }
        m_devices.clear();
        
        // Destroy instance
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    Device* Runtime::getDevice(size_t index)
    {
        if (index < m_devices.size()) {
            return m_devices[index].get();
        }
        return nullptr;
    }

    size_t Runtime::getDeviceCount() const
    {
        return m_devices.size();
    }

} // namespace runtime