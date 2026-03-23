//
// Created by void on 2026/3/22.
//

#ifndef CMONITPROTOCOL_H
#define CMONITPROTOCOL_H

#include "CProtocol.hpp"

struct SMsg {
    uint8_t type;
    int key;
    short len;
    uint8_t* data;
};
class CMonitProtocol : public CProtocol<SMsg>{
public:
    CMonitProtocol();
    ~CMonitProtocol();
private:

};



#endif //CMONITPROTOCOL_H
