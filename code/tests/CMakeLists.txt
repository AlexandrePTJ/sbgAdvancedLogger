project(sbgAdvancedLoggerTests)

add_executable(${PROJECT_NAME})

set(TEST_HELPERS_SRCS
        src/testHelpers/testTcpEchoServer.cpp
        src/testHelpers/testTcpEchoServer.h
)

target_sources(${PROJECT_NAME} PRIVATE
        ${TEST_HELPERS_SRCS}
        src/common/tcpConnectionTests.cpp
        src/common/utilsTests.cpp
        src/ntrip/ntripClientTests.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/testHelpers
)

target_link_libraries(${PROJECT_NAME} PRIVATE sbgAdvancedLoggerObj Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(${PROJECT_NAME})
