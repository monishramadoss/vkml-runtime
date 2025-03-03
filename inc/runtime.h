#pragma once

#include "config.h"
#include "debug_utils.h"
#include "error_handling.h"
#include "version.h"

#include <vulkan/vulkan.h>
#include <volk.h>
#include <vector>
#include <memory>

#include "vkd.h"
#include "vkb.h"
#include "vkc.h"
#include "vke.h"

namespace runtime {

class Instance {
public:
    Instance() {
        initializeInstance();
        enumerateDevices();
    }

    // Delete copy constructor and assignment
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    // Enable move semantics
    Instance(Instance&& other) noexcept
        : m_instance(other.m_instance)
        , m_devices(std::move(other.m_devices))
        , m_debugMessenger(other.m_debugMessenger)
    {
        other.m_instance = VK_NULL_HANDLE;
        other.m_debugMessenger = VK_NULL_HANDLE;
    }

    Instance& operator=(Instance&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_instance = other.m_instance;
            m_devices = std::move(other.m_devices);
            m_debugMessenger = other.m_debugMessenger;
            other.m_instance = VK_NULL_HANDLE;
            other.m_debugMessenger = VK_NULL_HANDLE;
        }
        return *this;
    }

    Device& get_device(uint32_t i = 0) { return m_devices.at(i); }
    const Device& get_device(uint32_t i = 0) const { return m_devices.at(i); }
    size_t get_device_count() const { return m_devices.size(); }

    ~Instance() { cleanup(); }

private:
    VkInstance m_instance{VK_NULL_HANDLE};
    std::vector<Device> m_devices;
    VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};
    
    static constexpr std::array<const char*, 1> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    void initializeInstance() {
        check_result(volkInitialize(), "Failed to initialize volk");

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = Version::APPLICATION_NAME;
        appInfo.applicationVersion = Version::APPLICATION_VERSION;
        appInfo.pEngineName = Version::ENGINE_NAME;
        appInfo.engineVersion = Version::ENGINE_VERSION;
        appInfo.apiVersion = Version::API_VERSION;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> extensions;
        setupDebugMessenger(createInfo, extensions);

        check_result(vkCreateInstance(&createInfo, nullptr, &m_instance),
                    "Failed to create instance");
        volkLoadInstance(m_instance);
    }

    void setupDebugMessenger(VkInstanceCreateInfo& createInfo,
                           std::vector<const char*>& extensions) {
#ifdef _DEBUG
        if (checkValidationLayerSupport()) {
            auto debugCreateInfo = DebugMessenger::getCreateInfo();
            createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.pNext = &debugCreateInfo;

            check_result(
                DebugMessenger::create(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger),
                "Failed to create debug messenger");
        }
#endif
    }

    bool checkValidationLayerSupport() const {
#ifdef _DEBUG
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        return std::all_of(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end(),
            [&](const char* layerName) {
                return std::any_of(availableLayers.begin(), availableLayers.end(),
                    [&](const auto& layerProperties) {
                        return strcmp(layerName, layerProperties.layerName) == 0;
                    });
            });
#else
        return false;
#endif
    }

    void enumerateDevices() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

        m_devices.reserve(deviceCount);
        for (auto& pd : physicalDevices) {
            m_devices.emplace_back(m_instance, pd);
        }
    }

    void cleanup() {
        if (m_instance != VK_NULL_HANDLE) {
            for (auto& dev : m_devices) {
                dev.destroy();  // Fixed typo in method name
            }
            m_devices.clear();

            if (m_debugMessenger != VK_NULL_HANDLE) {
                DebugMessenger::destroy(m_instance, m_debugMessenger, nullptr);
            }

            vkDestroyInstance(m_instance, nullptr);
            volkFinalize();
            m_instance = VK_NULL_HANDLE;
        }
    }
};



class App;
class Device;
class VulkanError;

class Runtime {
public:
    Runtime() { initialize(); }
    ~Runtime() { m_running = false; }

    void run(App& app);
    void stop(); 
    
    size_t get_device_count() const { return n_devices; }
    void submitWork(std::map<std::string, std::vector<uint32_t>>& work) {
        for(size_t i = 0; i < n_devices; ++i) { 
            m_devices[i].submitBlobs(work["blob_device_i"+std::to_string(i)]);
        }
        m_devices[for_device].submitBlobs(blobs);
        // copy engine stuff here
        
        std::vector<uint32_t> toDevice(0, n_devices);
        std::vector<uint32_t> fromDevice(0, n_devices);
        std::vector<std::vector<uint32_t>> copy_work;
        for(size_t i = 0; i < n_devices; ++i){
            for(size_t j = 0; j < n_devices; ++j){
                if(i != j){
                    copy_work.push_back(copy["copy_device_"+std::to_string(i)+"to_device_"+std::to_string(j)]);
                }
            }
            
       }

    }


private:
    void initialize();
    void enumerateDevices(std::vector<VkPhysicalDevice>& physicalDevices);

    bool m_running{false};

    std::unique_ptr<App> m_app{nullptr};
    std::unique_ptr<Device> m_devices{nullptr};
    std::unique_ptr<VulkanError> m_error{nullptr};
    size_t n_devices;
    
}

} // namespace runtime