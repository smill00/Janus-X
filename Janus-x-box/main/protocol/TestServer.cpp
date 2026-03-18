#include "TestServer.hpp"
CTestServer::CTestServer():m_protocoler (std::bind(&CTestServer::recvPacket, this, std::placeholders::_1)) {

}
CTestServer::~CTestServer() {

}

void CTestServer::recvPacket(const ProtocolCodec::Packet& packet) {
    if(m_connect_state == ProtocolCodec::ConnectState::CONNECTED) {
        // 处理已连接状态下的包
    } else if(m_connect_state == ProtocolCodec::ConnectState::LISTENING) {
        // 处理监听状态下的包
    } else {
        // 处理断开状态下的包
        switch(packet.type) {
            case ProtocolCodec::CommandType::HANDSHAKE:
                // 处理连接包
                break;
            
            default:
                break;
        }
    }
}

void CTestServer::changeState(ProtocolCodec::ConnectState state) {
    m_connect_state = state;
}

void CTestServer::handleHandshake(const ProtocolCodec::Packet& packet) {
    
}
