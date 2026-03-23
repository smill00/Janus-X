//
// Created by void on 2026/3/19.
//

#ifndef CPROTOCOL_H
#define CPROTOCOL_H

#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "CBlockQueue.hpp"
#include "CChannel.h"

template <typename T>
class CProtocol {
public:
    enum EByteSort {
        E_BIG,
        E_LATTLE
    };

    struct SProtocolConfig {
        EByteSort mode;
        uint8_t* head;
        int head_len;

        int fun_len;
        int len_index;
        int len_len;

        int len_max;
        int len_min;
    };

    struct SDataPacket {
        uint8_t* data;
        int data_len;

        uint8_t* fundata;
        int fun_len;

        uint8_t crc;
    };

    CProtocol(SProtocolConfig cfg) : m_channel(nullptr), m_running(false),m_cfg(cfg) {}
    virtual ~CProtocol(){}

    virtual T parse(SDataPacket packet) = 0;

    bool start() {
        m_worker = std::thread([this]()
        {
            enum ParseState
            {
                E_HEAD,
                E_LENGTH,
                E_BODY
            };

            ParseState state = E_HEAD;
            SDataPacket pkg;
            while (m_running)
            {
                switch (state)
                {
                case E_HEAD:
                    {
                        pkg = SDataPacket();
                        uint8_t head[m_cfg.head_len];
                        for (int i = 0; i < m_cfg.head_len; i++) {
                            if (m_channel->readData(head+i, 1) != 1 || head[i] != m_cfg.head[i]) {
                                break;
                            }
                            if (i == m_cfg.head_len - 1)
                            {
                                state = E_LENGTH;
                            }
                        }
                    }
                    break;
                case E_LENGTH:
                    {
                        uint8_t data[m_cfg.fun_len];
                        if (m_channel->readnData(data, m_cfg.fun_len) != m_cfg.fun_len)
                        {
                            state = E_HEAD;
                            break;
                        }
                        msg.len = data[0] << 8 | data[1];
                        if (msg.len < 0 || msg.len > 1024)
                        {
                            state = E_HEAD;
                            break;
                        }
                        msg.type = data[2];
                        msg.key = (uint32_t)data[4] << 24 | (uint32_t)data[5] << 16 | (uint32_t)data[6] << 8 | (
                            uint32_t)data[7];
                        state = E_BODY;
                    }
                    break;
                case E_BODY:
                    {
                        msg.data = new uint8_t[msg.len];
                        if (m_channel->readnData(msg.data, msg.len) != msg.len)
                        {
                            if (msg.data != nullptr)
                            {
                                delete [] msg.data;
                            }
                            state = E_HEAD;
                            break;
                        }
                    }
                }
            }
        });
        return true;
    }

    bool stop() { return true; }

private:
    std::thread m_worker;
    CChannel* m_channel;
    bool m_running;
    BlockingQueue<T> m_queue;

    SProtocolConfig m_cfg;
};

#endif //CPROTOCOL_H
