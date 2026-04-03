idf_component_register(
        SRCS
        "main.cpp"
        "protocol/autotest_protocol.cpp"
        "protocol/TestServer.cpp"
        "protocol/CChannel.cpp"
        "protocol/CChannelEspIdfUart.cpp"
        "protocol/CMonitProtocol.cpp"
        "protocol/CConfig.cpp"
        "protocol/CUploadProtocol.cpp"
        "driver/CDs3231.cpp"

        "protocol/CBlockQueue.hpp"
        "protocol/CProtocol.hpp"

        INCLUDE_DIRS
        "."
        "protocol"
        "driver"
)