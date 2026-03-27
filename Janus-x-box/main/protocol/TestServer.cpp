#include "TestServer.hpp"

#include "arch/sys_arch.h"
#include "CDs3231.h"
#include "CConfig.hpp"


server::server()
{
    m_protocol = new CMonitProtocol;
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
            if (m_state == DISCONNECTED || m_state == LISTENING)
            {
                if (msg.type != HANDSHAKE) {
                    continue;
                }
            }

            switch (msg.type) {
            case HANDSHAKE:
                {
                    handshake(msg);
                }break;
            case START_LISTEN:
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

    SMsg res_msg;
    uint8_t * res_data = nullptr;

    if (cfg.mode == LISTEN_SERIAL) {
        SUartConfig uart_cfg = CConfig::readUartConfig();

        res_data = new uint8_t[8];
        res_msg.len = 8;
        res_msg.data = res_data;
        res_msg.nc = LISTEN_SERIAL;
        res_msg.key = message.key;

        memcpy(res_data, &(uart_cfg.baud_rate), 4);
        res_data[4] = uart_cfg.stop_bit;
        res_data[5] = uart_cfg.parity;


    }


    delete message.data;
}


