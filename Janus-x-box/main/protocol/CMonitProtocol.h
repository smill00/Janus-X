//
// Created by void on 2026/3/22.
//

#ifndef CMONITPROTOCOL_H
#define CMONITPROTOCOL_H

#include "ProtocolCore.hpp"

struct SMsg {
    uint8_t type;
    int key;
    short len;
    uint8_t nc;
    uint8_t* data;
};

/**
 * @brief Janus-X 协议功能码 (type) 枚举定义
 * 此枚举严格对应《Janus-X协议.pdf》中“type:功能码”表格。
 */
 enum EFunCode{
    HANDSHAKE      = 0x0a, /**< 握手，建立连接，建立成功进入待机状态 */
    START_LISTEN   = 0x0b, /**< 启动监听，从待机状态进入监听状态 */
    STOP_LISTEN    = 0x0c, /**< 从监听状态复位到待机状态 */
    CONFIG_SERIAL  = 0x0d, /**< 下发串口参数并且切换到串口模式 */
    CONFIG_ETHERNET = 0x0e, /**< 下发网口参数并且切换到以太网模式 */
    CONFIG_AP      = 0x0f, /**< 下发AP服务器监听参数并且切换到AP模式 */
    DEVICE_RESET   = 0x10, /**< 设备主动上报复位，用于按键、意外断开后同步状态给PC */
    UNKNOWN        = 0xFF, /**< 未知类型，用于错误处理或初始化 */
};

static uint8_t HEAD[3] = {0x5A,0x1A,0x5A};

class CMonitProtocol : public CProtocol<SMsg>{
public:
    CMonitProtocol(Channel* ch);
    ~CMonitProtocol();

    SMsg decode(SDataPacket &packet) override;
    uint8_t* encode(SMsg &msg, int *len) override;
    SProtocolConfig protocolCfg()override;
private:

};



#endif //CMONITPROTOCOL_H
