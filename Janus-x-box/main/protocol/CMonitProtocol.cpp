//
// Created by void on 2026/3/22.
//

#include "CMonitProtocol.h"

CMonitProtocol::CMonitProtocol():
CProtocol(SProtocolConfig{E_BIG, HEAD,3}) {}

CMonitProtocol::~CMonitProtocol()
{
}

SMsg CMonitProtocol::parse(const SDataPacket& packet) {
    SMsg msg;
    msg.type = packet.fundata[2];
    msg.key = build_key_from_bytes(packet.fundata+4, 4, E_BIG);
    msg.data = packet.data;
    msg.len = packet.data_len;
    return msg;
}
