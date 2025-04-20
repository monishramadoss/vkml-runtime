#pragma once
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <iostream>

namespace vkrt {
namespace test {

// Simple assertion macro for tests
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("ASSERTION FAILED: " message); \
        } \
    } while (false)

// Base test class
class Test {
public:
    Test(std::string name) : m_name(std::move(name)) {}
    virtual ~Test() = default;
    virtual void setup() {}
    virtual void teardown() {}
    virtual void run() = 0;
    std::string getName() const { return m_name; }
protected:
    std::string m_name;
};

// Test registry for collecting and running tests
class TestRegistry {
public:
    static TestRegistry& getInstance();
    void addTest(Test* test);
    int runAllTests();
private:
    std::vector<Test*> m_tests;
};

// Macro to register a test class
#define REGISTER_TEST(TestClass) \
    static bool TestClass##_registered = []() { \
        vkrt::test::TestRegistry::getInstance().addTest(new TestClass(#TestClass)); \
        return true; \
    }();

} // namespace test
} // namespace vkrt
