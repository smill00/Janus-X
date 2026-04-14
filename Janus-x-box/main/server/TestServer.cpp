#include "TestServer.hpp"

#include "arch/sys_arch.h"
#include "CDs3231.h"
#include "CConfig.hpp"


server::server() {
    m_protocol = new CMonitProtocol(nullptr);
}

server::~server() {
    if (m_protocol) {
        m_protocol->reset();
        delete m_protocol;
    }
}

bool server::start()
{
    m_protocol->start();
    m_worker = std::thread([this]()
    {
        while (true) {
            SMsg msg = m_protocol->pull();
            if (msg.type != HANDSHAKE && m_state == DISCONNECTED) {
                continue;
            }

            switch (msg.type) {
            case HANDSHAKE:
                {
                    handshake(msg);
                }break;
            case CONFIG_SERIAL:
                {
                    handshake(msg);
                }break;

            default:break;
            }
        }
    });
    return true;
}

void server::reset() {
    m_state = DISCONNECTED;
    sys_msleep(500);
    m_protocol->reset();
}

void server::handshake(SMsg& message) {
    if (!message.data) { return; }

    ds3231_time_t time;
    time.year = message.data[0];
    time.month = message.data[1];
    time.day = message.data[2];
    time.hours = message.data[3];
    time.minutes = message.data[4];
    time.seconds = message.data[5];
    time.seconds = message.data[6] << 8 | message.data[7];

    CDs3231::ins().setTime(&time);
    m_protocol->reset();

    SSystemConfig cfg = CConfig::readSystemConfig();

    SMsg res_msg{};
    res_msg.nc = cfg.mode;
    res_msg.key = message.key;
    res_msg.len = cfg.mode == LISTEN_SERIAL ? 8 : 10;
    res_msg.data = new uint8_t[res_msg.len];

    if (cfg.mode == LISTEN_SERIAL) {
        SUartConfig uart_cfg = CConfig::readUartConfig();
        memcpy(res_msg.data, &(uart_cfg.baud_rate), 4);
        res_msg.data[4] = uart_cfg.stop_bit;
        res_msg.data[5] = uart_cfg.parity;
        res_msg.data[6] = cfg.ch_def;
        res_msg.data[7] = 0x01;
    } else if (cfg.mode == LISTEN_AP) {
        NetConfig net_cfg = CConfig::readNetConfig(cfg.mode == LISTEN_AP ? AP_CONFIG_NAME : ETH_CONFIG_NAME);
        int index = 0;
        memcpy(res_msg.data+index, &(net_cfg.ip), 4);  index += 4;
        memcpy(res_msg.data+index, &(net_cfg.mac), 6); index += 6;
        memcpy(res_msg.data+index, &(net_cfg.gateway), 4);  index += 4;
        memcpy(res_msg.data+index, &(net_cfg.netmask), 4);  index += 4;
        memcpy(res_msg.data+index, &(net_cfg.dns), 4);  index += 4;
        res_msg.data[index] = cfg.ch_def; index += 1;
        res_msg.data[index] = 0x01;
    }

    m_protocol->push(res_msg);
    delete message.data;
    m_state = STANDBY;
}

void server::handConfigUart(SMsg& message) {
    SUartConfig uart_cfg;
    int index = 0;
    uart_cfg.baud_rate = CUtils::intFromBytes(message.data, 4);
    uart_cfg.stop_bit = message.data[4];
    uart_cfg.parity = message.data[5];

    SSystemConfig sys_cfg = CConfig::readSystemConfig();
    if (sys_cfg.ch_def != message.data[6])
    {
        sys_cfg.ch_def = message.data[6];
        CConfig::writeSystemConfig(sys_cfg);
    }

    CConfig::writeUartConfig(uart_cfg);

}


