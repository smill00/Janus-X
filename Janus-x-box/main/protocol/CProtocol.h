//
// Created by void on 2026/3/19.
//

#ifndef CPROTOCOL_H
#define CPROTOCOL_H

#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "CChannel.h"

class CProtocol {
public:
    CProtocol();
    ~CProtocol();

    bool start();
    bool stop();

private:
    std::thread m_worker;
    CChannel* m_channel;
    bool m_running;
};

#endif //CPROTOCOL_H
