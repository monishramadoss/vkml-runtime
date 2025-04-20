#include "test_utils.h"
#include <vector>
#include <iostream>
#include <stdexcept>

namespace vkrt {
namespace test {

// TestRegistry implementation
TestRegistry& TestRegistry::getInstance() {
    static TestRegistry instance;
    return instance;
}

void TestRegistry::addTest(Test* test) {
    m_tests.push_back(test);
}

int TestRegistry::runAllTests() {
    int passed = 0;
    int failed = 0;
    std::cout << "Running " << m_tests.size() << " tests" << std::endl;
    for (auto test : m_tests) {
        try {
            std::cout << "[ RUN      ] " << test->getName() << std::endl;
            test->setup();
            test->run();
            test->teardown();
            std::cout << "[       OK ] " << test->getName() << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cerr << "[  FAILED  ] " << test->getName() << std::endl;
            std::cerr << "Error: " << e.what() << std::endl;
            failed++;
        }
    }
    std::cout << "\nTest results: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed;
}

} // namespace test
} // namespace vkrt
