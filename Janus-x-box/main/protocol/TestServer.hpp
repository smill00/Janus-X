// #ifndef TESTSERVER_HPP
// #define TESTSERVER_HPP
// #include "autotest_protocol.hpp"
// class CTestServer {
// public:
//     CTestServer();
//     ~CTestServer();
//
//     void recvPacket(const ProtocolCodec::Packet& packet);
// protected:
//     void changeState(ProtocolCodec::ConnectState state);
//     void handleHandshake(const ProtocolCodec::Packet& packet);
//
// private:
//     ProtocolCodec::ConnectState m_connect_state{ProtocolCodec::ConnectState::DISCONNECTED};
//     ProtocolCodec m_protocoler;
// };
//
// #endif // TESTSERVER_HPP
