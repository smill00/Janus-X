//
// Created by panshiquan on 2026/3/19.
//

#include "CChannel.h"

#include <cstring>
#include <chrono>
#include <unistd.h>



CChannel::CChannel(int buf_size, int read_size) :
m_buf_size(buf_size),
m_read_size(read_size),
m_buffer(new char[buf_size]) {}

CChannel::~CChannel() {
    m_is_run = false;
    m_data_available_cv.notify_all();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    delete[] m_buffer;
}

bool CChannel::run() {
    if (m_thread.joinable()) {
        return true;
    }

    if (m_is_run || !init()) {
        return false;
    }

    m_is_run = true;
    m_thread = std::thread([this]() {
        while (m_is_run) {
            char data[m_read_size];
            memset(data, 0, m_read_size);
            int bytes_read = readToBuf(data, m_read_size);
            if (bytes_read > 0) {
                std::lock_guard<std::mutex> lock(m_mutex);

                if (m_buf_size - m_write_pos < bytes_read) {
                    if ((m_buf_size - m_write_pos + m_read_pos) > (bytes_read * 3)) {
                        memmove(m_buffer, m_buffer + m_read_pos, m_write_pos - m_read_pos);
                        m_write_pos = m_write_pos - m_read_pos;
                        m_read_pos = 0;
                    } else {
                        char* new_buffer = new char[m_buf_size * 2];
                        memset(new_buffer, 0, m_buf_size * 2);
                        memcpy(new_buffer, m_buffer+m_read_pos, m_write_pos-m_read_pos);
                        m_write_pos = m_write_pos-m_read_pos;
                        m_read_pos = 0;
                        delete [] m_buffer;
                        m_buffer = new_buffer;
                        m_buf_size = m_buf_size * 2;
                    }
                }

                memcpy(m_buffer+m_write_pos, data, bytes_read);
                m_write_pos += bytes_read;

                m_data_available_cv.notify_one();
            } else if (bytes_read == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } else {
                m_is_run = false;
            }
        }
        reset();
    });
    return true;
}

bool CChannel::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_is_run = false;
    m_read_pos = 0;
    m_write_pos = 0;
    m_data_available_cv.notify_one();
    return true;
}

int CChannel::readData(uint8_t *data, int maxlen) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_data_available_cv.wait(lock, [this]() {
        return m_write_pos > m_read_pos || !m_is_run;
    });

    if (!m_is_run) {
        return -1;
    }

    int read_len = m_write_pos - m_read_pos >= maxlen ? maxlen : m_write_pos - m_read_pos;
    memcpy(data, m_buffer + m_read_pos, read_len);
    m_read_pos += read_len;

    if(m_buf_size - m_write_pos < m_read_size * 3) {
        memmove(m_buffer, m_buffer + m_read_pos, m_write_pos - m_read_pos);
        m_write_pos = m_write_pos - m_read_pos;
        m_read_pos = 0;
    }

    return read_len;
}

int CChannel::readnData(uint8_t* data, int len) {
    int total_read = 0;
    while (total_read < len) {
        int read_len = readData(data + total_read, len - total_read);
        if (read_len <= 0) {
            return total_read;
        }
        total_read += read_len;
    }
    return len;
}


#if TARGE_PLATFORM_POSIX
bool CChannel::writeData(const char *data, int len) {
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
#endif
