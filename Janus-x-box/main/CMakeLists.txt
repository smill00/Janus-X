idf_component_register(
        SRCS
        "main.cpp"
        "protocol/autotest_protocol.cpp"
        "protocol/TestServer.cpp"
        "protocol/uart_channel.cpp"

        INCLUDE_DIRS
        "."
        "protocol"
)