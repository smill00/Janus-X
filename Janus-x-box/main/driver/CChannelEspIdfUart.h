//
// Created by void on 2026/3/19.
//

#ifndef CCHANNELESPIDFUART_H
#define CCHANNELESPIDFUART_H

#include "Channel.hpp"
#include "driver/uart.h"
#include "esp_log.h"

class CChannelEspIdfUart : public Channel {
public:
    /**
     * @param uart_num       UART端口号 (UART_NUM_0, UART_NUM_1, UART_NUM_2)
     * @param tx_pin         发送引脚
     * @param rx_pin         接收引脚
     * @param baud_rate      波特率
     * @param buf_size       内部缓冲区大小
     * @param read_size      每次读取的数据大小
     */
    CChannelEspIdfUart(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate);
    CChannelEspIdfUart(const CChannelEspIdfUart&) = delete;
    CChannelEspIdfUart& operator=(const CChannelEspIdfUart&) = delete;

    std::string descr() override;

protected:
    bool init() override;
    bool unInit() override;

private:
    uart_port_t m_uart_num;
    int m_tx_pin;
    int m_rx_pin;
    int m_baud_rate;

    bool m_is_init = false;

    int m_esp_buf_size;

    static const char* TAG;
};

#endif //CCHANNELESPIDFUART_H