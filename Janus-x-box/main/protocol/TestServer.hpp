#ifndef TESTSERVER_HPP
#define TESTSERVER_HPP
#include "CMonitProtocol.h"
class server {
public:
    enum EState {
        DISCONNECTED,
        STANDBY,
        LISTENING
    };

    server();
    ~server();

    bool start();
    void stop();

private:
    void handleMessage(SMsg message);
    CMonitProtocol* m_protocol = nullptr;
    std::thread m_worker;
    EState m_state = DISCONNECTED;
};


#endif // TESTSERVER_HPP
