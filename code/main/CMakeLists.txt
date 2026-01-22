project(sbgAdvancedLogger)
set(PROJECT_OBJ_LIB_NAME ${PROJECT_NAME}Obj)

set(SRCS
        src/analyzer/abstractFormatAnalyzer.cpp
        src/analyzer/sbgEComAnalyzer.cpp
        src/common/httpGetContext.cpp
        src/common/nmea.cpp
        src/common/tcpConnection.cpp
        src/common/utils.cpp
        src/config/config.cpp
        src/file/bufferedWriter.cpp
        src/interfaces/abstractInterface.cpp
        src/interfaces/fileInterface.cpp
        src/interfaces/serialInterface.cpp
        src/interfaces/tcpInterface.cpp
        src/interfaces/udpInterface.cpp
        src/ntrip/ntripClient.cpp
        src/dataLogger.cpp
)

set(HDRS
        src/analyzer/abstractFormatAnalyzer.h
        src/analyzer/sbgEComAnalyzer.h
        src/common/httpGetContext.h
        src/common/nmea.h
        src/common/tcpConnection.h
        src/common/types.h
        src/common/utils.h
        src/config/config.h
        src/file/bufferedWriter.h
        src/interfaces/abstractInterface.h
        src/interfaces/fileInterface.h
        src/interfaces/serialInterface.h
        src/interfaces/tcpInterface.h
        src/interfaces/udpInterface.h
        src/ntrip/ntripClient.h
        src/dataLogger.h
)


add_library(${PROJECT_OBJ_LIB_NAME} OBJECT ${SRCS} ${HDRS})

target_include_directories(${PROJECT_OBJ_LIB_NAME}
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_link_libraries(${PROJECT_OBJ_LIB_NAME}
        PUBLIC
        asio
        fmt::fmt
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        sbgECom
)


set(EXECUTABLE_SRCS src/main.cpp)
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE $<TARGET_OBJECTS:${PROJECT_OBJ_LIB_NAME}> ${EXECUTABLE_SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_OBJ_LIB_NAME} CLI11::CLI11)
