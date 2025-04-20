#ifndef VERSION_H
#define VERSION_H

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

#include <string>

namespace runtime {

/**
 * @brief Version and build information management
 * 
 * Provides consistent versioning and identification for the runtime
 */
class Version {
public:
    // Application information
    static constexpr const char* APPLICATION_NAME = "Hello ML";
    static constexpr const char* ENGINE_NAME = "vkmlrt";
    static constexpr uint32_t APPLICATION_VERSION = VK_MAKE_VERSION(1, 0, 0);
    static constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(0, 0, 1);
    static constexpr uint32_t API_VERSION = VK_API_VERSION_1_3;
    
    // Build information
    #ifndef BUILD_TIMESTAMP
    static constexpr const char* BUILD_TIMESTAMP = __DATE__ " " __TIME__;
    #endif
    
    #ifdef DEBUG
    static constexpr bool IS_DEBUG_BUILD = true;
    #else
    static constexpr bool IS_DEBUG_BUILD = false;
    #endif

    /**
     * @brief Get string representation of a Vulkan version number
     * @param version VK_MAKE_VERSION-formatted version number
     * @return Formatted string (e.g., "1.2.3")
     */
    [[nodiscard]] static std::string getVersionString(uint32_t version) {
        return std::to_string(VK_VERSION_MAJOR(version)) + "." +
               std::to_string(VK_VERSION_MINOR(version)) + "." +
               std::to_string(VK_VERSION_PATCH(version));
    }
    
    [[nodiscard]] static std::string getApplicationVersionString() {
        return getVersionString(APPLICATION_VERSION);
    }
    
    [[nodiscard]] static std::string getEngineVersionString() {
        return getVersionString(ENGINE_VERSION);
    }
    
    [[nodiscard]] static std::string getApiVersionString() {
        return getVersionString(API_VERSION);
    }
    
    /**
     * @brief Get full build information string
     * @return String containing version and build details
     */
    [[nodiscard]] static std::string getBuildInfoString() {
        return std::string(ENGINE_NAME) + " v" + getEngineVersionString() + 
               " (" + BUILD_TIMESTAMP + ")" +
               (IS_DEBUG_BUILD ? " [DEBUG]" : " [RELEASE]");
    }
    
    /**
     * @brief Populate Vulkan application info structure
     * @return Configured VkApplicationInfo structure
     */
    [[nodiscard]] static VkApplicationInfo getApplicationInfo() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = APPLICATION_NAME;
        appInfo.applicationVersion = APPLICATION_VERSION;
        appInfo.pEngineName = ENGINE_NAME;
        appInfo.engineVersion = ENGINE_VERSION;
        appInfo.apiVersion = API_VERSION;
        return appInfo;
    }
};

} // namespace runtime

#endif // VERSION_H