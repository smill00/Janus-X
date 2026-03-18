// UartChannel.hpp
#pragma once

#include <functional>
#include <mutex>
#include <vector>
#include <atomic>
#include "driver/uart.h"
#include "esp_log.h"

/**
 * @brief 增强型 UART 通道类。
 * 职责：硬件管理、事件驱动、线程安全的数据发送。
 * 数据处理逻辑通过回调函数交由用户定义，实现控制反转。
 */
class UartChannel {
public:
    // 配置结构体
    struct Config {
        uart_port_t port_num = UART_NUM_1;
        int tx_pin = -1;
        int rx_pin = -1;
        int baud_rate = 115200;
        uart_word_length_t data_bits = UART_DATA_8_BITS;
        uart_parity_t parity = UART_PARITY_DISABLE;
        uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
        size_t rx_buffer_size = 1024;   // 驱动层RX缓冲区
        size_t tx_buffer_size = 0;      // 驱动层TX缓冲区 (0 为默认)
        size_t event_queue_size = 20;
        int rx_timeout = 3;             // 字符超时时间，用于触发数据到达回调
    };

    // 数据到达回调函数类型定义
    // 参数：数据指针，数据长度，指向本Channel实例的指针（用于在回调中得知数据来源）
    using DataReceivedCallback = std::function<void(const uint8_t* data, size_t len)>;

    /**
     * @brief 构造函数。
     * @param config UART 硬件配置。
     * @param callback 数据到达回调函数。当串口收到数据并成功读取后，会调用此函数。
     *                  注意：此回调在UART事件任务上下文中被调用，应保持简短，避免阻塞。
     */
    UartChannel(DataReceivedCallback callback);
    
    ~UartChannel();

    // 禁止拷贝
    UartChannel(const UartChannel&) = delete;
    UartChannel& operator=(const UartChannel&) = delete;

    bool begin(); // 初始化并启动
    void end();   // 停止并释放资源

    /**
     * @brief 线程安全的发送函数。
     * @param data 待发送数据的指针。
     * @param len 数据长度。
     * @return 实际发送的字节数（由ESP-IDF驱动返回），负数表示失败。
     */
    int send(const uint8_t* data, size_t len);

    /**
     * @brief 线程安全的发送函数（向量版本）。
     */
    int send(const std::vector<uint8_t>& data);

    /**
     * @brief 获取一个可用于发送的函数对象。
     *        此函数对象本身是线程安全的，可在任何任务中安全调用。
     * @return std::function<int(const uint8_t*, size_t)> 发送函数。
     */
    std::function<int(const uint8_t*, size_t)> get_sender() {
        // 返回一个捕获了`this`的lambda，其内部调用线程安全的send方法
        return [this](const uint8_t* data, size_t len) -> int {
            return this->send(data, len);
        };
    }

    /**
     * @brief 获取当前通道的配置。
     */
    const Config& get_config() const { return config_; }

    void setConfig(const Config& config);

    /**
     * @brief 检查通道是否活跃。
     */
    bool is_active() const { return is_active_.load(); }

    /**
     * @brief 获取错误计数（用于调试）。
     */
    uint32_t get_error_count() const { return error_count_.load(); }

    /**
     * @brief 立即清空硬件和软件接收缓冲区。
     */
    void flush_rx();

private:
    // 内部事件处理任务
    static void event_task(void* arg);
    void event_task_impl();

    Config config_;
    DataReceivedCallback user_callback_; // 用户设置的数据处理钩子
    QueueHandle_t uart_event_queue_ = nullptr;
    TaskHandle_t event_task_handle_ = nullptr;
    std::mutex tx_mutex_; // 发送互斥锁，确保多任务并发发送时的线程安全
    std::atomic<bool> is_active_{false};
    std::atomic<uint32_t> error_count_{0};
};