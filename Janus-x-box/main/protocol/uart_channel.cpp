// UartChannel.cpp
#include "uart_channel.hpp"
#include <cstring>

static const char* TAG = "UartChannel";

UartChannel::UartChannel(const Config& config, DataReceivedCallback callback)
    : config_(config), user_callback_(std::move(callback)) {
    // 构造函数主要进行配置校验和初始化
    if (!user_callback_) {
        ESP_LOGW(TAG, "UART%d: No callback provided. Data will be received but not processed.", config_.port_num);
    }
}

UartChannel::~UartChannel() {
    end();
}

bool UartChannel::begin() {
    if (is_active_.load()) {
        ESP_LOGW(TAG, "UART%d is already active.", config_.port_num);
        return true;
    }

    // 1. 配置UART参数
    uart_config_t uart_config = {
        .baud_rate = config_.baud_rate,
        .data_bits = config_.data_bits,
        .parity = config_.parity,
        .stop_bits = config_.stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t err = uart_param_config(config_.port_num, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d config params failed: %s", config_.port_num, esp_err_to_name(err));
        return false;
    }

    // 2. 设置引脚
    err = uart_set_pin(config_.port_num, config_.tx_pin, config_.rx_pin,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d set pins failed: %s", config_.port_num, esp_err_to_name(err));
        return false;
    }

    // 3. 安装驱动（关键：创建事件队列）
    err = uart_driver_install(config_.port_num,
                              config_.rx_buffer_size,
                              config_.tx_buffer_size,
                              config_.event_queue_size,
                              &uart_event_queue_,
                              0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART%d driver install failed: %s", config_.port_num, esp_err_to_name(err));
        uart_event_queue_ = nullptr;
        return false;
    }

    // 4. 设置接收超时
    // 此参数至关重要：它决定了`UART_DATA`事件在总线空闲多长时间后触发。
    // 设置较小值(如3个字符时间)可以更及时地回调，但可能将一个数据包拆分成多次回调。
    // 设置较大值有利于减少回调次数，但会增加处理延迟。
    uart_set_rx_timeout(config_.port_num, config_.rx_timeout);

    // 5. 创建事件处理任务
    BaseType_t task_created = xTaskCreate(UartChannel::event_task,
                                          "uart_evt",
                                          4096, // 可根据需要调整堆栈
                                          this,
                                          5,    // 优先级应高于业务逻辑，低于关键系统任务
                                          &event_task_handle_);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "UART%d create event task failed", config_.port_num);
        uart_driver_delete(config_.port_num);
        uart_event_queue_ = nullptr;
        return false;
    }

    is_active_.store(true);
    ESP_LOGI(TAG, "UART%d started. TX:GPIO%d, RX:GPIO%d, Baud:%d, Timeout:%d chars",
             config_.port_num, config_.tx_pin, config_.rx_pin,
             config_.baud_rate, config_.rx_timeout);
    return true;
}

void UartChannel::end() {
    bool expected = true;
    if (!is_active_.compare_exchange_strong(expected, false)) {
        return; // 已经处于非活跃状态
    }

    // 1. 删除事件任务
    if (event_task_handle_ != nullptr) {
        vTaskDelete(event_task_handle_);
        event_task_handle_ = nullptr;
        // 注意：这里等待一小段时间，确保任务已完全退出，避免后续访问成员变量
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 2. 卸载驱动
    if (config_.port_num >= UART_NUM_0 && config_.port_num < UART_NUM_MAX) {
        uart_driver_delete(config_.port_num);
    }
    uart_event_queue_ = nullptr;

    ESP_LOGI(TAG, "UART%d stopped.", config_.port_num);
}

int UartChannel::send(const uint8_t* data, size_t len) {
    if (!is_active_.load() || data == nullptr || len == 0) {
        return -1;
    }

    // 使用互斥锁确保多任务环境下发送操作的原子性
    std::lock_guard<std::mutex> lock(tx_mutex_);
    
    // uart_write_bytes 是线程安全的吗？ESP-IDF驱动内部有保护，但为防未来变更和确保逻辑清晰，此处仍加锁。
    // 此函数将数据拷贝至驱动的TX缓冲区后立即返回。
    int bytes_written = uart_write_bytes(config_.port_num, (const char*)data, len);
    
    // 可选：如果你需要确保数据全部从硬件FIFO发出，可以调用（但会阻塞）：
    // uart_wait_tx_done(config_.port_num, pdMS_TO_TICKS(1000));
    
    return bytes_written;
}

int UartChannel::send(const std::vector<uint8_t>& data) {
    return send(data.data(), data.size());
}

void UartChannel::flush_rx() {
    if (is_active_.load()) {
        uart_flush_input(config_.port_num);
    }
}

// --- 核心：事件处理任务 ---
void UartChannel::event_task(void* arg) {
    UartChannel* instance = static_cast<UartChannel*>(arg);
    if (instance) {
        instance->event_task_impl();
    }
    vTaskDelete(nullptr);
}

void UartChannel::event_task_impl() {
    uart_event_t event;
    const size_t temp_buf_size = 1024; // 每次读取的最大字节数
    uint8_t* temp_buf = static_cast<uint8_t*>(malloc(temp_buf_size));
    
    if (temp_buf == nullptr) {
        ESP_LOGE(TAG, "UART%d: Failed to allocate temp buffer for event task.", config_.port_num);
        is_active_.store(false);
        vTaskDelete(nullptr);
        return;
    }

    while (is_active_.load()) {
        // 阻塞等待UART事件
        if (xQueueReceive(uart_event_queue_, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case UART_DATA: {
                    // 这是最常见的事件：有数据到达或超时触发
                    size_t bytes_to_read = (event.size > 0) ? event.size : temp_buf_size;
                    bytes_to_read = (bytes_to_read > temp_buf_size) ? temp_buf_size : bytes_to_read;

                    // 从驱动缓冲区读取数据（非阻塞，因为数据已在缓冲区）
                    int len = uart_read_bytes(config_.port_num, temp_buf, bytes_to_read, 0);
                    
                    if (len > 0) {
                        // 调用用户设置的回调函数，传递数据和长度
                        if (user_callback_) {
                            try {
                                user_callback_(temp_buf, len);
                            } catch (const std::exception& e) {
                                ESP_LOGE(TAG, "UART%d: User callback threw exception: %s", config_.port_num, e.what());
                            } catch (...) {
                                ESP_LOGE(TAG, "UART%d: User callback threw unknown exception.", config_.port_num);
                            }
                        } else {
                            ESP_LOGW(TAG, "UART%d: %d bytes received but no callback set.", config_.port_num, len);
                        }
                    }
                    break;
                }
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART%d: HW/SW buffer overflow. Data may be lost.", config_.port_num);
                    uart_flush_input(config_.port_num);
                    error_count_++;
                    break;
                case UART_BREAK:
                    ESP_LOGI(TAG, "UART%d: Break signal detected.", config_.port_num);
                    if (user_callback_) {
                        // 可选：通过特殊数据（如空数据）或额外的事件回调通知用户BREAK事件
                        // user_callback_(nullptr, 0, this); // 示例
                    }
                    break;
                case UART_PARITY_ERR:
                case UART_FRAME_ERR:
                    ESP_LOGW(TAG, "UART%d: Frame error (type:%d).", config_.port_num, event.type);
                    error_count_++;
                    break;
                // 其他事件类型可以按需处理...
                default:
                    break;
            }
        }
    }

    free(temp_buf); // 释放临时缓冲区
}