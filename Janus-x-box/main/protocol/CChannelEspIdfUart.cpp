//
// Created by void on 2026/3/19.
//

#include "CChannelEspIdfUart.h"

const char* CChannelEspIdfUart::TAG = "CChannelEspIdfUart";

CChannelEspIdfUart::CChannelEspIdfUart(uart_port_t uart_num, int tx_pin, int rx_pin,int baud_rate, int buf_size, int read_size):
      CChannel(buf_size, read_size),
      m_uart_num(uart_num),
      m_tx_pin(tx_pin),
      m_rx_pin(rx_pin),
      m_baud_rate(baud_rate),
      m_esp_buf_size(buf_size)
{

}

std::string CChannelEspIdfUart::descr() {
    char buf[64];
    snprintf(buf, sizeof(buf), "ESP-IDF UART%d (TX:%d, RX:%d, %d bps)",
             m_uart_num, m_tx_pin, m_rx_pin, m_baud_rate);
    return std::string(buf);
}

bool CChannelEspIdfUart::init() {
    if (m_is_init) {
        unInit();
    }

    esp_err_t err;

    // 配置UART参数
    uart_config_t uart_config = {
        .baud_rate = m_baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    err = uart_param_config(m_uart_num, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return false;
    }

    // 设置UART引脚
    err = uart_set_pin(m_uart_num, m_tx_pin, m_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return false;
    }

    // 安装UART驱动（使用阻塞模式）
    err = uart_driver_install(m_uart_num,
                              m_esp_buf_size,  // 接收缓冲区大小
                              0,               // 发送缓冲区大小（不使用内部发送缓冲）
                              0,               // 队列大小
                              NULL,            // 队列句柄
                              0); // 中断分配标志
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "UART%d initialized successfully", m_uart_num);
    m_is_init = true;
    return true;
}

bool CChannelEspIdfUart::unInit() {
    m_is_init = false;
    esp_err_t err = uart_driver_delete(m_uart_num);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_delete failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "UART%d deinitialized", m_uart_num);
    return true;
}

int CChannelEspIdfUart::readToBuf(char* data, int size) {
    // ESP-IDF uart_read_bytes是阻塞调用，会等待直到有数据可读或超时
    // 使用portMAX_DELAY表示无限等待
    int length = uart_read_bytes(m_uart_num, (uint8_t*)data, size, portMAX_DELAY);

    if (length < 0) {
        ESP_LOGE(TAG, "uart_read_bytes error");
        return -1;
    }

    return length;
}
