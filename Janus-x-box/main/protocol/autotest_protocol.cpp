// ProtocolCodec.cpp
#include "autotest_protocol.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

void ProtocolCodec::pushBuffer(uint8_t* buffer, size_t len) {
    if(buffer == nullptr || len == 0) {
        return;
    }
    m_buffer.push(DataPkg(buffer, len));
}

void ProtocolCodec::sendMsg(const Packet& packet) {
    if(packet.value == nullptr || packet.len == 0) {
        return;
    }
    uint8_t* buf = (uint8_t*)malloc(packet.len + 8);
    if(buf == nullptr) {
        return;
    }

    memcpy(buf, PACKET_HEADER.data(), PACKET_HEADER.size());
    buf[3] = packet.len;
    buf[4] = packet.len >> 8;
    memcpy(buf + 5, &packet.key, sizeof(packet.key));
    memcpy(buf + 9, packet.value, packet.len);
    buf[9 + packet.len] = 0;
}

void ProtocolCodec::worker() {
    enum class ParseState {
            FIND_HEADER,
            PARSE_TYPE,
            PARSE_LENGTH,
            PARSE_KEY,
            PARSE_VALUE,
            PKGEND
    };
    ParseState state = ParseState::FIND_HEADER;
    uint8_t buf[512] = {0};
    int write_pos = 0;
    int read_pos = 0;
    Packet packet;
    while(true) {
        DataPkg pkg = m_buffer.pull();
        if(pkg.data == nullptr || pkg.len == 0) {
            continue;
        }
        memcpy(buf + write_pos, pkg.data, pkg.len);
        write_pos += pkg.len;
        if(pkg.data != nullptr) {
            free(pkg.data);
        }

        // 处理数据包
        switch(state) {
            case ParseState::FIND_HEADER:
                if(write_pos - read_pos >= 3) {
                    if(buf[read_pos] == '*' && buf[read_pos + 1] == '*' && buf[read_pos + 2] == '*') {
                        state = ParseState::PARSE_LENGTH;
                        read_pos += 3;
                    } else {
                        read_pos ++;
                        for(int i = 0; i < write_pos - read_pos; i++) {
                            if(buf[read_pos + i] == '*') {
                                break;
                            }
                            read_pos ++;
                        }
                        continue;
                    }
                }
                continue;
            case ParseState::PARSE_LENGTH:
                if(write_pos - read_pos >= 8) {
                     packet.len = (buf[read_pos] << 8) | buf[read_pos + 1];
                     packet.type = (CommandType)buf[read_pos + 2];
                     packet.key = (buf[read_pos + 4] << 24) | (buf[read_pos + 5] << 16) | (buf[read_pos + 6] << 8) | buf[read_pos + 7];
                     read_pos += 8;
                     state = ParseState::PARSE_VALUE;
                }
                continue;
            case ParseState::PARSE_VALUE:
                if(write_pos - read_pos >= packet.len) {
                    packet.value = (uint8_t*)malloc(packet.len);
                    memset(packet.value, 0, packet.len);
                    memcpy(packet.value, buf+read_pos, packet.len);

                    memcpy(packet.value, buf + read_pos, packet.len);
                    read_pos += packet.len;
                    state = ParseState::PKGEND;
                }
                continue;
            case ParseState::PKGEND:
                memcpy(buf, buf + read_pos, write_pos - read_pos);
                write_pos = write_pos - read_pos;
                read_pos = 0;
                m_push_packet_callback(packet);
                state = ParseState::FIND_HEADER;
                continue;
            default:
                state = ParseState::FIND_HEADER;
                break;
        }
    }
}
