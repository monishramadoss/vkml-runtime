#ifndef RUNTIME_DISPATCH_BUILDER_H
#define RUNTIME_DISPATCH_BUILDER_H

#include <memory>
#include <vulkan/vulkan.h>

namespace runtime {

class ComputeProgram;
class Buffer;
class Texture;
enum class BindingType;

/**
 * @brief Builder pattern for configuring and launching compute dispatches
 * 
 * This class provides a fluent interface for setting up and executing compute workloads,
 * handling synchronization, and resource binding.
 */
class DispatchBuilder {
public:
    /**
     * @brief Construct a dispatch builder for a specific compute program
     * @param program The compute program to dispatch
     */
    explicit DispatchBuilder(std::shared_ptr<ComputeProgram> program);
    
    /**
     * @brief Set the local workgroup dimensions
     * @return Builder reference for method chaining
     */
    DispatchBuilder& setWorkgroupSize(uint32_t x, uint32_t y = 1, uint32_t z = 1);
    
    /**
     * @brief Set the global workgroup count
     * @return Builder reference for method chaining
     */
    DispatchBuilder& setWorkgroupCount(uint32_t x, uint32_t y = 1, uint32_t z = 1);
    
    /**
     * @brief Bind a storage buffer to a descriptor set and binding
     * @return Builder reference for method chaining
     */
    DispatchBuilder& bindStorageBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);
    
    /**
     * @brief Bind a uniform buffer to a descriptor set and binding
     * @return Builder reference for method chaining
     */
    DispatchBuilder& bindUniformBuffer(uint32_t set, uint32_t binding, std::shared_ptr<Buffer> buffer);
    
    /**
     * @brief Bind a texture to a descriptor set and binding
     * @return Builder reference for method chaining
     */
    DispatchBuilder& bindTexture(uint32_t set, uint32_t binding, std::shared_ptr<Texture> texture, 
                               BindingType type);
    
    /**
     * @brief Set up for indirect dispatch using a buffer containing dispatch parameters
     * @return Builder reference for method chaining
     */
    DispatchBuilder& setIndirectBuffer(std::shared_ptr<Buffer> buffer, VkDeviceSize offset = 0);
    
    /**
     * @brief Set push constants for the dispatch
     * @return Builder reference for method chaining
     */
    DispatchBuilder& setPushConstants(const void* data, uint32_t size, uint32_t offset = 0);
    
    /**
     * @brief Execute the compute dispatch with current configuration
     * @throws std::runtime_error if no program set or configuration is invalid
     */
    void dispatch();
    
    /**
     * @brief Wait for dispatch completion
     * @param timeout Maximum time to wait in nanoseconds
     * @throws std::runtime_error if wait fails
     */
    void wait(uint64_t timeout = UINT64_MAX);
    
    /**
     * @brief Check if dispatch has completed
     * @return True if completed or nothing was dispatched
     * @throws std::runtime_error if no program set
     */
    bool isCompleted() const;
    
private:
    std::shared_ptr<ComputeProgram> m_program;
    
    // Workgroup configuration
    uint32_t m_workgroupSizeX{1};
    uint32_t m_workgroupSizeY{1};
    uint32_t m_workgroupSizeZ{1};
    uint32_t m_workgroupCountX{1};
    uint32_t m_workgroupCountY{1};
    uint32_t m_workgroupCountZ{1};
    
    // Indirect dispatch
    bool m_useIndirect{false};
    std::shared_ptr<Buffer> m_indirectBuffer;
    VkDeviceSize m_indirectOffset{0};
    
    // Push constants
    bool m_usePushConstants{false};
    const void* m_pushConstantData{nullptr};
    uint32_t m_pushConstantSize{0};
    uint32_t m_pushConstantOffset{0};
};

} // namespace runtime


#endif // RUNTIME_DISPATCH_BUILDER_H