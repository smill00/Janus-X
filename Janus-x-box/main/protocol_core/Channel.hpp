//
// Created by panshiquan on 2026/3/19.
//

#ifndef MYPROJECT_CCHANNEL_H
#define MYPROJECT_CCHANNEL_H
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <string>
#include <cstring>
#include <utility>
#include <unistd.h>

#include "Utils.hpp"

class Channel {
public:
    Channel(std::string name) : m_name(name), m_is_run(true) {}

    virtual ~Channel() {
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    bool readnData(uint8_t *data, int len) {
        if (len <= 0)
            return true;
        if (!m_is_run)
            return false;

        int received = 0;
        while (received < len) {
            int ret = readData(data + received, len - received);
            if (ret <= 0) {
                return false;
            }
            received += ret;
        }
        return true;
    }

    virtual int readData(uint8_t *data, int maxlen) {
        int len = read(m_fd, data, maxlen);
        return len;
    };

    bool writeData(const uint8_t *data, int len) {
        if (!m_is_run || len <= 0) {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_write_mutex);
        int total = 0;

        while (total < len && m_is_run) {
            int remaining = len - total;
            int length = write(m_fd, data + total, remaining);

            if (length > 0) {
                total += length;
            } else if (length == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } else {
                return false;  // 返回false
            }
        }

        return total == len ? true : false;
    }


    virtual std::string descr() = 0;

    virtual bool init() = 0;
    virtual bool unInit() = 0;
protected:


    int m_fd = -1;


protected:
    std::string m_name;
    std::atomic_bool m_is_run{false};
    std::mutex m_write_mutex;
};

#endif //MYPROJECT_CCHANNEL_H
