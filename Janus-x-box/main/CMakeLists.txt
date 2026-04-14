idf_component_register(
        SRCS
        "server/main.cpp"
        "server/CConfig.cpp"
        "server/TestServer.cpp"

        "protocol/CMonitProtocol.cpp"
        "protocol/CUploadProtocol.cpp"

        "driver/CDs3231.cpp"
        "driver/CChannelEspIdfUart.cpp"

        "protocol_core/BlockingQueu.hpp"
        "protocol_core/Channel.hpp"
        "protocol_core/json.hpp"
        "protocol_core/Utils.hpp"
        "protocol_core/ProtocolCore.hpp"

        INCLUDE_DIRS
        "driver"
        "protocol"
        "protocol_core"
        "server"
)