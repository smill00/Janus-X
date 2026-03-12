// ProtocolCodec.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <array>

/**
 * @brief Janus-X 协议编解码器。
 * 职责：完成协议数据包与字节流之间的双向转换，并验证帧格式、长度、CRC。
 * 不涉及状态管理、业务逻辑或硬件IO。
 */
class ProtocolCodec {
public:
    // 协议常量定义
    static constexpr std::array<uint8_t, 3> PACKET_HEADER = {'*', '*', '*'};
    static constexpr size_t MAX_PACKET_VALUE_LEN = 512;
    static constexpr uint8_t RESERVED_NC = 0x00;

    // 协议功能码 (Type)
    enum class CommandType : uint8_t {
        HANDSHAKE = 0x0A,
        START_LISTEN = 0x0B,
        STOP_LISTEN = 0x0C,
        CONFIG_UART = 0x0D,
        CONFIG_ETHERNET = 0x0E,
        CONFIG_AP = 0x0F,
        DEVICE_RESET = 0x10,
        // ... 可根据需要扩展
    };

    // 设备模式
    enum class DeviceMode : uint8_t {
        UART = 0x01,
        ETHERNET = 0x02,
        AP = 0x03,
    };

    // 通道定义
    enum class ChannelDef : uint8_t {
        CH1_TX_CH2_RX = 0x00,
        CH1_RX_CH2_TX = 0x01,
    };

    // 核心数据结构：表示一个完整的协议数据包
    struct Packet {
        CommandType type;          // 功能码
        uint32_t key;              // 随机密钥，用于时序匹配
        std::vector<uint8_t> value; // 可变长数据载荷
        bool isValid{false};       // 解析后标记是否有效

        // 以下为“数据包组装”所需的临时或内部字段，编码后丢弃
        // uint16_t dataLength; // 由value.size()计算得出
        // uint8_t crc;        // 在序列化时计算
    };

    // 针对特定功能码的、更结构化的数据体定义（用于便捷构造和解析）
    struct HandshakeRequest {
        uint32_t key;
        struct {
            uint16_t year;
            uint8_t month;
            uint8_t day;
            uint8_t hour;
            uint8_t minute;
            uint8_t second;
            uint16_t millisecond;
        } timestamp;
    };

    struct UartConfig {
        uint32_t baudRate;
        uint8_t stopBits;
        uint8_t parity;
        ChannelDef channelDef;
    };

    struct NetworkConfig {
        uint32_t ipAddress; // 注意网络字节序
        uint16_t ch1Port;
        uint16_t ch2Port;
        ChannelDef channelDef;
    };

public:
    ProtocolCodec() = default;
    ~ProtocolCodec() = default;

    // ===================== 核心编解码接口 =====================
    /**
     * @brief 从字节流解码出一个完整的协议包
     * @param data 输入的字节流起始指针
     * @param len  输入的字节流长度
     * @return std::optional<Packet> 解码成功返回Packet对象，失败返回std::nullopt
     */
    std::optional<Packet> decode(const uint8_t* data, size_t len) const;

    /**
     * @brief 将一个协议包编码为字节流
     * @param packet 输入的协议包对象
     * @return std::vector<uint8_t> 编码后的字节序列，失败则返回空vector
     */
    std::vector<uint8_t> encode(const Packet& packet) const;

    // ===================== 高级构造/解析工具函数 =====================
    // 这些函数封装了对Packet.value字段的解析，使业务逻辑更清晰
    static std::optional<HandshakeRequest> parseHandshakeRequest(const Packet& pkt);
    static std::optional<UartConfig> parseUartConfig(const Packet& pkt);
    static std::optional<NetworkConfig> parseNetworkConfig(const Packet& pkt);

    static Packet createHandshakeResponse(uint32_t key, DeviceMode mode, const std::vector<uint8_t>& params);
    static Packet createAckPacket(CommandType originalCmd, uint32_t key, bool isSuccess);
    static Packet createDeviceResetPacket();

private:
    // ===================== 内部工具函数 =====================
    /**
     * @brief 计算CRC8校验和（示例，需根据协议文档实现具体算法）
     * @param data 数据指针
     * @param len  数据长度
     * @return uint8_t CRC校验值
     */
    uint8_t calculateCrc(const uint8_t* data, size_t len) const;

    /**
     * @brief 验证帧头
     */
    bool verifyHeader(const uint8_t* data) const;

    /**
     * @brief 从字节流中提取uint16_t (假设协议为小端字节序)
     */
    uint16_t getU16Le(const uint8_t* data) const;
    void setU16Le(uint8_t* buffer, uint16_t value) const;
    // 类似函数: getU32Le, setU32Le...
};