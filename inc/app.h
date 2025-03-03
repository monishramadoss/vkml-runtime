#include <vulkan/vulkan.h>
#include <volk.h>


namespace runtime {
    class App {
        public :
            App();
            ~App();
            App(const App&) = delete;
            App& operator=(const App&) = delete;

            VkInstance getInstance() const { return m_instance; }

            void initalize();

        private:
           VkInstance m_instance;


    }
} // namespace runtime