// // ProtocolCodec.hpp
// #pragma once
//
// #include <cstdint>
// #include <functional>
// #include <array>
// #include <thread>
// #include <stdint.h>
//
// #include "ring_buffer.hpp"
//
// class ProtocolCodec {
// public:
//
//     static constexpr std::array<uint8_t, 3> PACKET_HEADER = {'*', '*', '*'};
//     static constexpr uint8_t MAX_PACKET_VALUE_LEN = 512;
//     static constexpr uint8_t RESERVED_NC = 0x00;
//
//     // 协议功能码 (Type)
//     enum class CommandType : uint8_t {
//         HANDSHAKE = 0x0A,
//         START_LISTEN = 0x0B,
//         STOP_LISTEN = 0x0C,
//         CONFIG_UART = 0x0D,
//         CONFIG_ETHERNET = 0x0E,
//         CONFIG_AP = 0x0F,
//         DEVICE_RESET = 0x10,
//         // ... 可根据需要扩展
//     };
//
//     // 设备模式
//     enum class DeviceMode : uint8_t {
//         UART = 0x01,
//         ETHERNET = 0x02,
//         AP = 0x03,
//     };
//
//     enum class ConnectState : uint8_t {
//         DISCONNECTED = 0x01,
//         CONNECTED = 0x02,
//         LISTENING = 0x03,
//     };
//
//     // 通道定义
//     enum class ChannelDef : uint8_t {
//         CH1_TX_CH2_RX = 0x00,
//         CH1_RX_CH2_TX = 0x01,
//     };
//
//     struct Packet {
//         CommandType type;
//         uint32_t key;
//         unsigned short len = 0;
//         uint8_t* value;
//
//
//     };
//
//     // 针对特定功能码的、更结构化的数据体定义（用于便捷构造和解析）
//     struct HandshakeRequest {
//         uint32_t key;
//         struct {
//             uint16_t year;
//             uint8_t month;
//             uint8_t day;
//             uint8_t hour;
//             uint8_t minute;
//             uint8_t second;
//             uint16_t millisecond;
//         } timestamp;
//     };
//
//     struct UartConfig {
//         uint32_t baudRate;
//         uint8_t stopBits;
//         uint8_t parity;
//         ChannelDef channelDef;
//     };
//
//     struct NetworkConfig {
//         uint32_t ipAddress;
//         uint16_t ch1Port;
//         uint16_t ch2Port;
//         ChannelDef channelDef;
//     };
//
//     struct DataPkg {
//         uint8_t* data;
//         size_t len;
//         DataPkg(const uint8_t* data, size_t len) {
//             this->data = (uint8_t*)data;
//             this->len = len;
//         }
//
//         DataPkg() = default;
//     };
//
//     using PushPacketCallback = std::function<void(const Packet packet)>;
// public:
//     ProtocolCodec(PushPacketCallback push_callback);
//
//     ~ProtocolCodec() ;
//
//     void pushBuffer(const uint8_t* buffer, size_t len);
//     void worker();
//
//     void sendMsg(const Packet& packet);
//
// private:
//     RingBuffer<DataPkg> m_buffer;
//     std::thread m_worker;
//     PushPacketCallback m_push_packet_callback;
//     //UartChannel m_channel;
// };
