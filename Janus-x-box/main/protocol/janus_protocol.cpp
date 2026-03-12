// ProtocolCodec.cpp
#include "janus_protocol.hpp"
#include <algorithm>
#include <cstring>

std::optional<ProtocolCodec::Packet> ProtocolCodec::decode(const uint8_t* data, size_t len) const {
    // 1. 基础长度检查
    const size_t MIN_PACKET_LEN = 3 + 2 + 1 + 1 + 4 + 0 + 1; // head+len+type+nc+key+value+crc
    if (len < MIN_PACKET_LEN) {
        return std::nullopt;
    }

    // 2. 验证帧头
    if (!verifyHeader(data)) {
        return std::nullopt;
    }

    // 3. 获取数据长度字段
    uint16_t dataLength = getU16Le(data + 3); // 跳过3字节head
    if (dataLength > MAX_PACKET_VALUE_LEN) { // 长度字段非法
        return std::nullopt;
    }

    // 4. 计算整个帧应有的长度
    size_t totalPacketLen = 3 + 2 + dataLength + 1; // head + len + data(type+nc+key+value) + crc
    if (len < totalPacketLen) { // 数据不完整
        return std::nullopt;
    }

    // 5. 提取并验证CRC
    uint8_t receivedCrc = data[totalPacketLen - 1];
    uint8_t calculatedCrc = calculateCrc(data + 3, 2 + dataLength); // 对 (len + data) 计算CRC
    if (receivedCrc != calculatedCrc) {
        return std::nullopt;
    }

    // 6. 解析数据区
    const uint8_t* dataSection = data + 5; // 指向 type 字段
    Packet packet;
    packet.type = static_cast<CommandType>(dataSection[0]);
    // dataSection[1] 是 nc，保留，跳过
    //packet.key = getU32Le(dataSection + 2); // 假设key是4字节小端

    // 7. 提取value
    size_t valueLen = dataLength - (1 + 1 + 4); // dataLength - (type+nc+key)
    if (valueLen > 0) {
        packet.value.assign(dataSection + 6, dataSection + 6 + valueLen);
    }
    packet.isValid = true;

    return packet;
}

std::vector<uint8_t> ProtocolCodec::encode(const Packet& packet) const {
    std::vector<uint8_t> buffer;
    buffer.reserve(3 + 2 + 1 + 1 + 4 + packet.value.size() + 1); // 预分配

    // 1. 帧头
    buffer.insert(buffer.end(), PACKET_HEADER.begin(), PACKET_HEADER.end());

    // 2. 数据区长度 (type(1) + nc(1) + key(4) + value.size())
    uint16_t dataLength = 1 + 1 + 4 + static_cast<uint16_t>(packet.value.size());
    buffer.push_back(static_cast<uint8_t>(dataLength & 0xFF));
    buffer.push_back(static_cast<uint8_t>((dataLength >> 8) & 0xFF));

    // 3. 数据区
    buffer.push_back(static_cast<uint8_t>(packet.type)); // type
    buffer.push_back(RESERVED_NC);                       // nc
    // key (小端)
    buffer.push_back(static_cast<uint8_t>(packet.key & 0xFF));
    buffer.push_back(static_cast<uint8_t>((packet.key >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((packet.key >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((packet.key >> 24) & 0xFF));
    // value
    buffer.insert(buffer.end(), packet.value.begin(), packet.value.end());

    // 4. 计算并附加CRC (对 len字段+data区 计算)
    uint8_t crc = calculateCrc(buffer.data() + 3, 2 + dataLength);
    buffer.push_back(crc);

    return buffer;
}

// 示例：解析握手请求
std::optional<ProtocolCodec::HandshakeRequest> ProtocolCodec::parseHandshakeRequest(const Packet& pkt) {
    if (pkt.type != CommandType::HANDSHAKE || !pkt.isValid || pkt.value.size() < 2+1+1+1+1+1+2) {
        return std::nullopt;
    }
    HandshakeRequest req;
    req.key = pkt.key;
    const uint8_t* v = pkt.value.data();
    //req.timestamp.year = getU16Le(v);
    req.timestamp.month = v[2];
    // ... 依次解析其他字段
    return req;
}

// 示例：创建握手响应包
ProtocolCodec::Packet ProtocolCodec::createHandshakeResponse(uint32_t key, DeviceMode mode, const std::vector<uint8_t>& params) {
    Packet pkt;
    pkt.type = CommandType::HANDSHAKE;
    pkt.key = key;
    pkt.value.clear();
    pkt.value.push_back(static_cast<uint8_t>(mode));
    pkt.value.insert(pkt.value.end(), params.begin(), params.end());
    pkt.isValid = true;
    return pkt;
}