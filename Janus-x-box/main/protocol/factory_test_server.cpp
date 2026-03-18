#include "factory_test_server.hpp"
#include "errno.h"

CFactoryTestServer::CFactoryTestServer() : m_test_manage(
    [this](const uint8_t* data, size_t len) { return this->uartSend(data, len); },
    [this](const uint8_t* data, size_t len) { return this->vsocketSend(data, len); }
) {
    return;
}

CFactoryTestServer::~CFactoryTestServer(){

}

void CFactoryTestServer::vsocketInit () {

}
bool CFactoryTestServer::serialPortInit() {
    if (m_uart_ctx.is_initialized && m_uart_ctx.fd >= 0) {
        return true; // 已经初始化
    }

    int fd = open(UART_PORT, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        syslog(LAYER_LOG_EER, "Failed to open serial port");
        return false;
    }

    // 配置串口参数
    struct termios options;
    if (tcgetattr(fd, &options) != 0) {
        syslog(LAYER_LOG_EER, "tcgetattr failed: %s", strerror(errno));
        close(fd);
        return false;
    }

    // 设置波特率

    if (cfsetispeed(&options, UART_BAUD_RATE)!= 0 || cfsetospeed(&options, UART_BAUD_RATE)) {
        syslog(LAYER_LOG_EER, "Failed to set baud rate %d: %s", UART_BAUD_RATE, strerror(errno));
        close(fd);
        return false;
    }

    // 8N1 配置
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // 启用接收和本地模式
    options.c_cflag |= (CLOCAL | CREAD);

    // 完全禁用硬件和软件流控制
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    // 原始输入模式
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // 原始输出模式
    options.c_oflag &= ~OPOST;

    // 设置阻塞模式超时
    options.c_cc[VMIN] = 0;        // 最小读取0字节
    options.c_cc[VTIME] = 5;       // 5 * 100ms = 0.5秒超时

    // 应用配置
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        syslog(LAYER_LOG_EER, "Failed to set serial port attributes");
        close(fd);
        return false;
    }

    // 清空输入输出缓冲区
    tcflush(fd, TCIOFLUSH);

    m_uart_ctx.fd = fd;
    m_uart_ctx.is_initialized = true;
    
    // 初始化互斥锁
    pthread_mutex_init(&m_uart_ctx.lock, NULL);
    
    // 启动串口接收线程
    startUartReceiveThread();

    return true;
}

ssize_t CFactoryTestServer::receiveBytes(uint8_t* buf, size_t max_len, int timeout_ms) {
    struct pollfd pfd = {
        .fd = m_server_ctx.client_fd,
        .events = POLLIN | POLLERR | POLLHUP | POLLRDHUP,
        .revents = 0
    };

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret == 0) {
        return 0;
    }
    if (ret < 0) {
        return -1;
    }

    if (pfd.revents & (POLLERR | POLLHUP | POLLRDHUP)) {
        return -1;
    }
    
    return read(m_server_ctx.client_fd, buf, max_len);
} 

// 串口接收 
ssize_t CFactoryTestServer::uartReceive(uint8_t* buffer, size_t buffer_size) {
    if (!m_uart_ctx.is_initialized || m_uart_ctx.fd < 0) {
        return -1;
    }

    ssize_t bytes_read = read(m_uart_ctx.fd, buffer, buffer_size);

    if (bytes_read < 0) {
        syslog(LAYER_LOG_EER, "UART read error: %s", strerror(errno));
        return -1;
    }

    return bytes_read;
}

void CFactoryTestServer::run() {
    struct sockaddr_vm addr = {
        .svm_family = AF_VSOCK,
        .svm_port = VSOCK_PORT,
        .svm_cid = VMADDR_CID_ANY,
        .svm_zero = {0}
    };

#ifdef BUILD_HYPERVISOR_UOS_TBOX
    /*AF_VSOCKX is using for both UOS socket connection*/
    m_server_ctx.server_fd = socket(AF_VSOCKX, SOCK_STREAM, 0);
#else
    m_server_ctx.server_fd = socket(AF_VSOCK, SOCK_STREAM, 0);
#endif

    if (m_server_ctx.server_fd < 0) {
        syslog(LAYER_LOG_EER, "socket creation failed\n");
        clear();
        return;
    }

    int opt = 1;

    if (setsockopt(m_server_ctx.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LAYER_LOG_EER, "setsockopt failed");
        clear();
    }

    if (bind(m_server_ctx.server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        syslog(LAYER_LOG_EER, "bind failed");
        clear();
    }

    if (listen(m_server_ctx.server_fd, 1) < 0) {
        syslog(LAYER_LOG_EER, "listen failed");
        clear();
    }

    syslog(LAYER_LOG_INFO, "VSOCK server listening on port %d\n", VSOCK_PORT);

    while (true) {
        struct sockaddr_vm client_addr;
        socklen_t addr_len = sizeof(client_addr);

        m_server_ctx.client_fd = accept(m_server_ctx.server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (m_server_ctx.client_fd < 0) {
            syslog(LAYER_LOG_EER, "accept failed");
            continue;
        }

        syslog(LAYER_LOG_INFO, "Client connected: CID=%u\n", client_addr.svm_cid);
        m_server_ctx.buffer_pos = 0;

        while (true) {
            size_t remaining_space = MAX_BUFFER_SIZE - m_server_ctx.buffer_pos;
            if (remaining_space < MAX_PACKET_SIZE) {
                processBuffef();
                remaining_space = MAX_BUFFER_SIZE - m_server_ctx.buffer_pos;
            }

            ssize_t bytes_read = receiveBytes(m_server_ctx.buffer + m_server_ctx.buffer_pos, remaining_space, CLIENT_TIMEOUT_MS);
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    continue;
                } else {
                    syslog(LAYER_LOG_EER, "close old fd");
                }
                break;
            }

            syslog(LAYER_LOG_EER, "receive vsock client %s\n", CProtocol::bytes2HexStringFast((const unsigned char*)(m_server_ctx.buffer + m_server_ctx.buffer_pos),bytes_read,' ',1));
            m_server_ctx.buffer_pos += bytes_read;
            processBuffef();
        }

        close(m_server_ctx.client_fd);
        m_server_ctx.client_fd = -1;
        syslog(LAYER_LOG_EER, "Client disconnected\n");
    }
    clear();
}

void CFactoryTestServer::clear() {
    if (m_server_ctx.server_fd >= 0) {
        close(m_server_ctx.server_fd);
        m_server_ctx.server_fd = -1;
    }
    if (m_server_ctx.client_fd >= 0) {
        close(m_server_ctx.client_fd);
        m_server_ctx.client_fd = -1;
    }
}

void CFactoryTestServer::processBuffef() {
    while (m_server_ctx.buffer_pos > 0) {
        size_t consumed = 0;
        MessageEntity* request = CProtocol::findSocketPackage(m_server_ctx.buffer, m_server_ctx.buffer_pos, &consumed);

        if (request) {
            pthread_t thread;
            if (request->data_len == 0) {
                syslog(LAYER_LOG_EER, "delete invalid package");

                CProtocol::freeMessage(request);
                if (consumed < m_server_ctx.buffer_pos) {
                    memmove(m_server_ctx.buffer,
                           m_server_ctx.buffer + consumed,
                           m_server_ctx.buffer_pos - consumed);
                    m_server_ctx.buffer_pos -= consumed;
                } else {
                    m_server_ctx.buffer_pos = 0;
                }

                continue; // 继续处理下一个包
            }

            m_test_manage.pushMsg(request);

            if (consumed < m_server_ctx.buffer_pos) {
                memmove(m_server_ctx.buffer, m_server_ctx.buffer + consumed, m_server_ctx.buffer_pos - consumed);
                m_server_ctx.buffer_pos -= consumed;
            } else {
                m_server_ctx.buffer_pos = 0;
            }
        } else {
            if (m_server_ctx.buffer_pos >= MAX_BUFFER_SIZE) {
                bool has_partial_header = false;
                for (size_t i = 0; i < HEAD && i < m_server_ctx.buffer_pos; i++) {
                    if (m_server_ctx.buffer[m_server_ctx.buffer_pos - i - 1] == HEADER1) {
                        has_partial_header = true;
                        break;
                    }
                }

                if (has_partial_header) {
                    size_t keep_size = HEAD;
                    memmove(m_server_ctx.buffer, m_server_ctx.buffer + m_server_ctx.buffer_pos - keep_size, keep_size);
                    m_server_ctx.buffer_pos = keep_size;
                    syslog(LAYER_LOG_EER, "Buffer full, kept partial header\n");
                } else {
                    m_server_ctx.buffer_pos = 0;
                    syslog(LAYER_LOG_EER, "Buffer full, no valid header found. Cleared buffer.\n");
                }
            }
            break;
        }
    }
}

// 串口发送（线程安全）
bool CFactoryTestServer::uartSend(const uint8_t* data, size_t len) {
    if (!m_uart_ctx.is_initialized || m_uart_ctx.fd < 0) {
        if (!serialPortInit()) {
            return false;
        }
    }

    pthread_mutex_lock(&m_uart_ctx.lock);

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(m_uart_ctx.fd, data + sent, len - sent);
        if (n <= 0) {
            pthread_mutex_unlock(&m_uart_ctx.lock);
            return false;
        }
        sent += n;
    }

    pthread_mutex_unlock(&m_uart_ctx.lock);
    return true;
}

bool CFactoryTestServer::vsocketSend(const uint8_t* data, size_t len) {
    pthread_mutex_lock(&m_server_ctx.write_mutex);

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(m_server_ctx.client_fd, data + sent, len - sent);
        if (n <= 0) {
            pthread_mutex_unlock(&m_server_ctx.write_mutex);
            return false;
        }
        sent += n;
    }

    pthread_mutex_unlock(&m_server_ctx.write_mutex);
    return true;
}

void CFactoryTestServer::startUartReceiveThread() {
    // 检查线程是否已经在运行
    if (m_uart_ctx.handle_thread.joinable()) {
        return;
    }
    
    // 创建串口接收线程
    m_uart_ctx.handle_thread = std::thread(&CFactoryTestServer::uartReceiveHandle, this);
    
    syslog(LAYER_LOG_INFO, "UART receive thread started");
}

void CFactoryTestServer::uartReceiveHandle() {
    const int MAX_REPEAT_COUNT = 3; // 最大允许重复次数
    while (m_uart_ctx.is_initialized) {
        // 计算剩余空间
        size_t remaining_space = MAX_BUFFER_SIZE - m_uart_ctx.buffer_pos;

        // 如果剩余空间小于最大包大小，处理数据腾出空间
        if (remaining_space < MAX_PACKET_SIZE) {
            processUartBuffer();
            remaining_space = MAX_BUFFER_SIZE - m_uart_ctx.buffer_pos;
        }

        // 接收数据
        ssize_t bytes_read = uartReceive(
            m_uart_ctx.buffer + m_uart_ctx.buffer_pos,
            remaining_space
        );
        
        if ( m_uart_ctx.buffer_pos > 0) {
            char* hex_str = CProtocol::bytes2HexStringFast(m_uart_ctx.buffer, m_uart_ctx.buffer_pos, ' ', 1);
            syslog(LAYER_LOG_INFO, "Received: %s\n", hex_str);
            free(hex_str);
        }

        if (bytes_read > 0) {
            m_uart_ctx.buffer_pos += bytes_read;
            //更新last缓冲区
            memcpy(m_uart_ctx.last_buffer, m_uart_ctx.buffer, m_uart_ctx.buffer_pos);
            m_uart_ctx.last_buffer_size = m_uart_ctx.buffer_pos;
            m_uart_ctx.repeat_count = 0;

            // 处理数据
            processUartBuffer();
        } else if (bytes_read == 0) {
            // 检查缓冲区是否与上次相同
            if (m_uart_ctx.buffer_pos > 0 &&
                m_uart_ctx.buffer_pos == m_uart_ctx.last_buffer_size &&
                memcmp(m_uart_ctx.buffer, m_uart_ctx.last_buffer, m_uart_ctx.buffer_pos) == 0) {

                // 增加重复计数
                m_uart_ctx.repeat_count++;

                syslog(LAYER_LOG_WARNING, "Buffer repeated %d times", m_uart_ctx.repeat_count);

                // 检查是否达到最大重复次数
                if (m_uart_ctx.repeat_count >= MAX_REPEAT_COUNT) {
                    syslog(LAYER_LOG_WARNING,
                          "Buffer repeated %d times, clearing buffer",
                          m_uart_ctx.repeat_count);
                    // 完全清除缓冲区
                    m_uart_ctx.buffer_pos = 0;
                    syslog(LAYER_LOG_INFO, "Cleared buffer after %d repeats", m_uart_ctx.repeat_count);

                    // 重置重复计数器
                    m_uart_ctx.repeat_count = 0;

                    // 更新参考缓冲区
                    m_uart_ctx.last_buffer_size = 0;
                }
            }
            else {
                // 保存当前缓冲区状态作为新的参照
                if (m_uart_ctx.buffer_pos > 0) {
                    memcpy(m_uart_ctx.last_buffer, m_uart_ctx.buffer, m_uart_ctx.buffer_pos);
                    m_uart_ctx.last_buffer_size = m_uart_ctx.buffer_pos;
                }

                // 重置重复计数器
                m_uart_ctx.repeat_count = 0;
            }
        } else if (bytes_read < 0) {
            syslog(LAYER_LOG_EER, "UART receive error");
        }
    }
    syslog(LAYER_LOG_EER, "UART thread exit");
    return;
}

void CFactoryTestServer::processUartBuffer() {
    char* printfString = NULL;
    size_t current_pos = 0; // 追踪当前处理位置
    while (current_pos < m_uart_ctx.buffer_pos) {
        size_t consumed = 0;
        MessageEntity* response = CProtocol::findUartPackagePackage(
            m_uart_ctx.buffer + current_pos,
            m_uart_ctx.buffer_pos - current_pos,
            &consumed
        );

        if (response) {
            size_t data_len = response->data_len;
            // 处理校验错误包（data_len == 0）
            if (response->data_len == 0) {
                syslog(LAYER_LOG_EER, "delete invalid package");
                size_t package_end = current_pos + consumed;

                if (package_end < m_uart_ctx.buffer_pos) {
                    memmove(m_uart_ctx.buffer + current_pos,
                           m_uart_ctx.buffer + package_end,
                           m_uart_ctx.buffer_pos - package_end);
                    m_uart_ctx.buffer_pos -= consumed;
                } else {
                    m_uart_ctx.buffer_pos = current_pos;
                }

                CProtocol::freeMessage(response);
                continue; // 继续处理下一个包
            }

            // 编码响应消息
            size_t encoded_len;
            if(response->type == 0){
                m_test_manage.cleanCmd(response->data[0]);
            }
            uint8_t* encoded = CProtocol::encodeMessage(response, data_len, &encoded_len);
            printfString = CProtocol::bytes2HexStringFast((const unsigned char*)encoded,encoded_len,' ',1);
            syslog(LAYER_LOG_EER, "uart send to android %s\n",printfString);
            free(printfString);

            if (encoded) {
                // 发送给PC
                vsocketSend(encoded, encoded_len);
                free(encoded);
            }

            // 移除整个包区域
            size_t package_end = current_pos + consumed;

            if (package_end < m_uart_ctx.buffer_pos) {
                // 关键修复：正确移动数据
                memmove(m_uart_ctx.buffer + current_pos,
                       m_uart_ctx.buffer + package_end,
                       m_uart_ctx.buffer_pos - package_end);
                m_uart_ctx.buffer_pos -= consumed;
            } else {
                // 处理的是最后一个包
                m_uart_ctx.buffer_pos = current_pos;
            }

            CProtocol::freeMessage(response);
        } else {
            if (consumed > 0) {
                // 找到不完整包位置，跳过它
                syslog(LAYER_LOG_INFO, "Skipping incomplete package at position %zu",
                      current_pos);
                current_pos += consumed;
                continue; // 继续尝试下一个位置
            } else {
                // 没有找到包，但保留剩余数据
                syslog(LAYER_LOG_INFO, "No package found, keeping %zu bytes",
                      m_uart_ctx.buffer_pos - current_pos);

                // 缓冲区满处理
                if (m_uart_ctx.buffer_pos >= MAX_BUFFER_SIZE) {
                    // 保留可能的包头部分
                    size_t keep_size = HEAD;
                    memmove(m_uart_ctx.buffer,
                           m_uart_ctx.buffer + m_uart_ctx.buffer_pos - keep_size,
                           keep_size);
                    m_uart_ctx.buffer_pos = keep_size;
                    current_pos = 0; // 重置处理位置
                    syslog(LAYER_LOG_INFO, "Buffer full, kept last %zu bytes", keep_size);
                    break;
                }
                break;
            }
        }
    }
}
