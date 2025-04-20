#include "logging.h"
#include "test_utils.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace runtime;
using namespace vkrt::test;

class LoggingTest : public Test {
public:
    LoggingTest(std::string name) : Test(name) {}
    void run() override {
        auto& logger = Logger::getInstance();
        logger.setLevel(LogLevel::DEBUG);
        logger.clearMessages();
        logger.setConsoleOutput(true);
        logger.debug("Debug message");
        logger.info("Info message");
        logger.warning("Warning message");
        logger.error("Error message");
        const auto& messages = logger.getMessages();
        TEST_ASSERT(!messages.empty(), "No log messages were recorded");
        TEST_ASSERT(messages.size() == 4, "Incorrect number of log messages");
        logger.clearMessages();
        TEST_ASSERT(logger.getMessages().empty(), "Messages were not cleared");
    }
};
REGISTER_TEST(LoggingTest);
