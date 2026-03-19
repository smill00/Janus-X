//
// Created by void on 2026/3/19.
//

#include "CProtocol.h"

CProtocol::CProtocol()
{

}
CProtocol::~CProtocol()
{

}

bool CProtocol::start() {
    m_worker = std::thread([this](){
        while (m_running) {
            char buf[100] = {0};
            m_channel->readData(buf, 100);

        }
    });
    return true;
}

bool CProtocol::stop()
{
    return true;
}
