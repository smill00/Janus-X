//
// Created by panshiquan on 2026/3/19.
//

#ifndef MYPROJECT_CCHANNEL_H
#define MYPROJECT_CCHANNEL_H
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#define TARGE_PLATFORM_POSIX false

class CChannel {
public:
    CChannel(int buf_size, int read_size = 100);
    virtual ~CChannel();
    CChannel(const CChannel&) = delete;
    CChannel& operator=(const CChannel&) = delete;

    int readData(uint8_t *data, int maxlen);
    int readnData(uint8_t *data, int len);

#if TARGE_PLATFORM_POSIX
    bool writeData(const char *data, int len);
#else
    virtual bool writeData(const char *data, int len) = 0;
#endif

    virtual std::string descr() = 0;
    bool run();
    bool reset();

protected:
    virtual int readToBuf(char* data, int size) = 0;  // 从数据源读取到内部缓冲区
    virtual bool init() = 0;
    virtual bool unInit() = 0;

#if TARGE_PLATFORM_MCU
    int m_fd = -1;
#endif

    std::atomic_bool m_is_run = false;

private:
    int m_read_pos = 0;
    int m_write_pos = 0;

    int m_buf_size = 0;
    int m_read_size = 0;
    char* m_buffer = nullptr;

    std::thread m_thread;
    std::mutex m_mutex;                           // 保护缓冲区和指针
    std::condition_variable m_data_available_cv;  // 数据可用条件变量
    std::mutex m_write_mutex;
};

#endif //MYPROJECT_CCHANNEL_H