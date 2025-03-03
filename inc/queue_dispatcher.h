#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <volk.h>


namespace runtime {
    class  QueueDispatcher {
    public:
        QueueDispatcher(uint32_t idx, VkDevice device);
        ~QueueDispatcher();

        // Prevent copying
        QueueDispatcher(const QueueDispatcher&) = delete;
        QueueDispatcher& operator=(const QueueDispatcher&) = delete;

        // Allow moving
        QueueDispatcher(QueueDispatcher&&) noexcept = default;
        QueueDispatcher& operator=(QueueDispatcher&&) noexcept = default;

        void submit(const ComputePacket& packet);
        void destroy();

        bool isSparseBindingFamily() const { return m_sparseBindingFamily; }
        uint32_t get_idx() const { return m_idx; }
}