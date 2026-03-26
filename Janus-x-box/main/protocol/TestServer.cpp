#include "TestServer.hpp"


server::server()
{
    m_protocol = new CMonitProtocol;
}

server::~server() {
    if (m_protocol) {
        m_protocol->stop();
        delete m_protocol;
    }
}

bool server::start() {
    m_protocol->start();
    m_worker = std::thread([this](){
        SMsg msg = m_protocol->pull();
        handleMessage(msg);
    });
    return true;
}

void server::stop()
{

}

void server::handleMessage(SMsg message)
{

}
