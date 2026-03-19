idf_component_register(
        SRCS
        "main.cpp"
        "protocol/autotest_protocol.cpp"
        "protocol/TestServer.cpp"
        "protocol/CChannel.cpp"
        "protocol/CChannelEspIdfUart.cpp"
        "protocol/CProtocol.cpp"

        INCLUDE_DIRS
        "."
        "protocol"
)