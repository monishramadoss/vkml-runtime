#ifndef APP_H
#define APP_H
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>

namespace runtime {
    // Forward declarations
    class Device;
    class DebugMessenger;

    class App {
    public:
        App();
        ~App();
        
        // Prevent copying
        App(const App&) = delete;
        App& operator=(const App&) = delete;

        VkInstance getInstance() const { return m_instance; }
        
        // Create a device
       
        std::vector<std::shared_ptr<Device>> pullDevices() const {return m_devices;}
        void initialize();
        void cleanup();

    private:
        void setupDebugMessenger();
        VkInstance m_instance{nullptr};
        std::unique_ptr<DebugMessenger> m_debugMessenger {nullptr};
        bool m_initialized{false};
        std::vector<std::shared_ptr<Device>> m_devices;
    };
} // namespace runtime

#endif // APP_H