//
// Created by void on 2026/3/22.
//

#include "CMonitProtocol.h"

CMonitProtocol::CMonitProtocol():
CProtocol(SProtocolConfig{
    E_BIG, HEAD,3,

})
{
}

CMonitProtocol::~CMonitProtocol()
{
}

SMsg CMonitProtocol::parse(const SDataPacket& packet)
{

}
