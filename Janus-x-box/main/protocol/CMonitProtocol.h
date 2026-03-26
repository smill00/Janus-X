//
// Created by void on 2026/3/22.
//

#ifndef CMONITPROTOCOL_H
#define CMONITPROTOCOL_H

#include "CProtocol.hpp"

struct SMsg {
    uint8_t type;
    int key;
    int len;
    uint8_t* data;
};

static uint8_t HEAD[3] = {0x5A,0x1A,0x5A};

class CMonitProtocol : public CProtocol<SMsg>{
public:
    CMonitProtocol();
    ~CMonitProtocol();

    SMsg parse(const SDataPacket& packet) override;
private:

};



#endif //CMONITPROTOCOL_H
