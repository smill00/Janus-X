#ifndef FEATURE_TEST_SERVER_H
#define FEATURE_TEST_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <pthread.h>
#include <poll.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <thread>

#include "protocol_yocto.hpp"
#include "factory_test_manage.hpp"


#define VSOCK_PORT 9000
#define CLIENT_TIMEOUT_MS 100 // 100ms超时
#define UART_PORT "/dev/ttyS1"
#define UART_BAUD_RATE B921600
#define UART_TIMEOUT_MS 1000 // 1s超时


struct SerialConfig{
    char name[32];
};

typedef struct {
    int fd;
    pthread_mutex_t lock;
    uint8_t buffer[MAX_BUFFER_SIZE];
    size_t buffer_pos;
    bool is_initialized;
    uint8_t last_buffer[MAX_BUFFER_SIZE];   // 保存上一次的缓冲区内容
    size_t last_buffer_size;                // 上一次的缓冲区大小
    int repeat_count;                       // 重复次数计数器

    std::thread handle_thread;
} SerialPortContext;

typedef struct {
    int server_fd;
    int client_fd;
    pthread_mutex_t write_mutex;
    uint8_t buffer[MAX_BUFFER_SIZE];
    size_t buffer_pos;
} TcpServerContext;

class CFactoryTestServer
{
public:
    CFactoryTestServer();
    ~CFactoryTestServer();

    void run();
    void clear();
    void processBuffef();
    void processUartBuffer();

    ssize_t receiveBytes(uint8_t* buf, size_t max_len, int timeout_ms);
    ssize_t uartReceive(uint8_t* buffer, size_t buffer_size);
  
    bool uartSend(const uint8_t* data, size_t len);
    bool vsocketSend(const uint8_t* data, size_t len);
    
protected:
    void vsocketInit ();
    bool serialPortInit();
    void startUartReceiveThread();
    void uartReceiveHandle();
private:
    CTestManage m_test_manage;
    SerialPortContext m_uart_ctx;
    TcpServerContext m_server_ctx;
};

#endif// FEATURE_TEST_SERVER_H