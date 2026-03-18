#ifndef FACTORY_TEST_MANAGE_H
#define FACTORY_TEST_MANAGE_H

#include <mutex>
#include <queue>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "protocol_yocto.hpp"


#define CMD_DELAY_US 1000000 // 1s超时
#define PWM_DELAY_US 1000000 // 1S超时

using sendUartCallback = std::function<bool(const uint8_t* data, size_t len)>;
using sendSocketCallback = std::function<bool(const uint8_t* data, size_t len)>;

class CTestItem;
class CTestManage {
public:
    CTestManage(sendUartCallback puart, sendSocketCallback psmsocket);
    ~CTestManage();

    void init();
    void start();
    void pushMsg(MessageEntity* msg);

    void applyCmd(int code);
    void cleanCmd(int code);
protected:
    void messageLoop();
    void processMsg(MessageEntity* request);
    
    int write_to_file(const char* file_path, const char* cmd);
    int run_command_exit_code(const char* cmd);

private:
    std::thread m_worker;
    std::queue<MessageEntity*> m_msgs;
    std::mutex m_msgs_mutex;
    std::condition_variable m_msg_cv;
    std::atomic<bool> m_running {false};

    sendUartCallback m_uart_sender;
    sendSocketCallback m_socket_sender;

    std::unordered_map<int, int> m_code_device_map;
    std::unordered_map<int, CTestItem*> m_test_items;
};

#endif