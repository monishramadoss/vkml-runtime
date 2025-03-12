#ifndef RUNTIME_H
#define RUNTIME_H

#include "config.h"
#include "debug_utils.h"
#include "error_handling.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <map>
#include <string>

namespace runtime {

class Device;
class App;


class Runtime {
public:
    Runtime();
    ~Runtime();

    void run(App& app);
    void stop();
    
    size_t getDeviceCount() const { return m_devices.size(); }
    
    /**
     * @brief Submit work items to appropriate devices
     * @param work Map of device-specific compute blobs and copy operations
     */
    void submitWork(const std::map<std::string, std::vector<uint32_t>>& work) ;
private:
    void initialize();
    void enumerateDevices(std::vector<VkPhysicalDevice>& physicalDevices);
    void cleanup();

    bool m_running{false};

    std::unique_ptr<App> m_app{nullptr};
    std::vector<std::unique_ptr<Device>> m_devices;
};

} // namespace runtime

#endif // RUNTIME_H