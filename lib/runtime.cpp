#include "runtime.h"
#include "app.h"
#include "device.h"

namespace runtime
{
    void Runtime::initialize()
    {
        m_app = std::make_unique<App>();
        std::vector<VkPhysicalDevice> physicalDevices;
        enumerateDevices(physicalDevices);
        n_devices = physicalDevices.size();
        m_devices = std::make_unique<Device>(new Device(n_devices));
        for (size_t i = 0; i < n_devices; ++i)
        {
            m_devices[i] = Device(m_instance, physicalDevices[i]);
        }

        m_running = true;
        m_logging = 

    }

    void Runtime::run(App& app)
    {
        m_running = true;
        m_app = &app;
        while (m_running)
        {
            m_app->update();
        }
    }

    void Runtime::stop()
    {
        m_running = false;
    }

    void Runtime::enumerateDevices(std::vector<VkPhysicalDevice>& physicalDevices)
    {
        n_devices = 0;
        vkEnumeratePhysicalDevices(m_instance, &n_devices, nullptr);
        physicalDevices.resize(n_devices);
        vkEnumeratePhysicalDevices(m_instance, &n_devices, physicalDevices.data());
    }


} // namespace vkrt