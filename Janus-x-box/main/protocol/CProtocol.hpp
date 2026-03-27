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

    virtual T parse(const SDataPacket& packet) = 0;
    virtual T pull() {
        return m_queue.pop();
    }

    bool start() {
        m_worker = std::thread([this]()
        {
            enum ParseState
            {
                E_HEAD,
                E_FUN,
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
                                state = E_FUN;
                            }
                        }
                    }break;
                case E_FUN:
                    {
                        if (m_channel->readnData(pkg.fundata, m_cfg.fun_len) != m_cfg.fun_len) {
                            state = E_HEAD;
                            break;
                        }

                        pkg.data_len = build_key_from_bytes(pkg.fundata+m_cfg.len_index, m_cfg.len_len, m_cfg.mode);
                        state = E_BODY;
                    }break;
                case E_BODY:
                    {
                        if (m_channel->readnData(pkg.data, pkg.data_len) != pkg.data_len)
                        {
                            if (pkg.fundata != nullptr)
                            {
                                delete [] pkg.fundata;
                            }
                            state = E_HEAD;
                            break;
                        }

                        T msg = parse(pkg);
                        m_queue.push(msg);
                        state = E_HEAD;
                    }break;
                }
            }
        });
        return true;
    }

    bool reset() {
        m_channel->reset();
        return true;
    }

    int build_key_from_bytes(const uint8_t* data, size_t byte_count, EByteSort mode) {
        int result = 0;

        if (byte_count > 4) {
            byte_count = 4; // 限制最多4字节
        }

        if (mode == E_BIG) { // 大端序
            for (size_t i = 0; i < byte_count; i++) {
                result |= (int)data[i] << (8 * (byte_count - 1 - i));
            }
        } else { // 小端序
            for (size_t i = 0; i < byte_count; i++) {
                result |= (int)data[i] << (8 * i);
            }
        }

        return result;
    }

private:
    std::thread m_worker;
    CChannel* m_channel;
    bool m_running;
    BlockingQueue<T> m_queue;

    SProtocolConfig m_cfg;
};

#endif //CPROTOCOL_H
