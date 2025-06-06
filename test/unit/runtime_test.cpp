#include "common/test_utils.hpp"
#include "runtime.h"

namespace vkml {
namespace test {

// Individual test functions that return true for pass, false for fail

bool testInitializationSuccess() {
    Runtime runtime;
    TEST_ASSERT(runtime.initialize(), "Runtime initialization failed");
    return true;
}

bool testMultipleInitializationHandling() {
    Runtime runtime;
    TEST_ASSERT(runtime.initialize(), "First runtime initialization failed");
    // Second initialization should fail or be handled gracefully
    bool secondInitResult = runtime.initialize();
    TEST_ASSERT(!secondInitResult, "Second initialization should fail or be handled gracefully");
    return true;
}

bool testShutdownSuccess() {
    Runtime runtime;
    TEST_ASSERT(runtime.initialize(), "Runtime initialization failed");
    runtime.shutdown();
    // After shutdown, initialization should work again
    TEST_ASSERT(runtime.initialize(), "Re-initialization after shutdown failed");
    return true;
}

bool testVersionRetrieval() {
    Runtime runtime;
    TEST_ASSERT(runtime.initialize(), "Runtime initialization failed");
    auto version = runtime.getVersion();
    TEST_ASSERT(version.major > 0, "Version major should be greater than 0");
    return true;
}

} // namespace test
} // namespace vkml

// Main function that runs all tests
int main() {
    bool allTestsPassed = true;
    
    // Run all runtime tests
    allTestsPassed &= vkml::test::runTest(vkml::test::testInitializationSuccess, "Runtime Initialization Success");
    allTestsPassed &= vkml::test::runTest(vkml::test::testMultipleInitializationHandling, "Multiple Initialization Handling");
    allTestsPassed &= vkml::test::runTest(vkml::test::testShutdownSuccess, "Shutdown Success");
    allTestsPassed &= vkml::test::runTest(vkml::test::testVersionRetrieval, "Version Retrieval");
    
    // Return appropriate exit code
    return allTestsPassed ? 0 : 1;
}
