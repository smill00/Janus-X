//
// Created by void on 2026/3/22.
//

#include "CMonitProtocol.h"

#include <cstring>

CMonitProtocol::CMonitProtocol():
CProtocol(SProtocolConfig{E_BIG, HEAD,3}) {}

CMonitProtocol::~CMonitProtocol()
{
}

SMsg CMonitProtocol::decode(const SDataPacket& packet) {
    SMsg msg;
    msg.type = packet.fundata[2];
    msg.key = build_key_from_bytes(packet.fundata+4, 4, E_BIG);
    msg.data = packet.data;
    msg.len = packet.data_len;
    return msg;
}

bool CMonitProtocol::encode(SMsg& msg, uint8_t* data, int* len) {
    data = new uint8_t[3+2+6+msg.len+1];
    int index = 0;
    memcpy(data+index, HEAD, 3);  index += 3;
    memcpy(data+index, &(msg.len), 2);  index += 2;
    memcpy(data+index, &(msg.type), 1);  index += 1;
    memcpy(data+index, &(msg.nc), 1);  index += 1;
    memcpy(data+index, &(msg.key), 4);  index += 4;
    memcpy(data+index, msg.data, msg.len);  index += msg.len;

    uint8_t checksum = calculateChecksum(data+3, msg.len+8);
    data[index] = checksum;

    *len = (3+2+6+msg.len+1);

    return true;
}
