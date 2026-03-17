#include "factory_test_manage.hpp"
#include "item_i2c.hpp"
#include "item_spi.hpp"
#include "item_usb.hpp"

CTestManage::CTestManage(sendUartCallback puart, sendSocketCallback psmsocket) : 
m_uart_sender(puart),
m_socket_sender(psmsocket),
m_running(false) {
}

CTestManage::~CTestManage() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    m_msg_cv.notify_all();  // 唤醒等待的线程
    
    if (m_worker.joinable()) {
        m_worker.join();  // 等待线程结束
    }
    
    // 清理剩余消息
    std::lock_guard<std::mutex> lock(m_msgs_mutex);
    while (!m_msgs.empty()) {
        MessageEntity* msg = m_msgs.front();
        m_msgs.pop();
        delete msg;  // 记得释放内存
    }
}

void CTestManage::init() {
    m_test_items.clear();
    m_test_items[SCP_I2C2] = new CTestItemI2C(SCP_I2C2, 17);
    m_test_items[SCP_I2C3] = new CTestItemI2C(SCP_I2C3, 18);
    m_test_items[SCP_I2C4] = new CTestItemI2C(SCP_I2C4, 19);
    m_test_items[SCP_I2C6] = new CTestItemI2C(SCP_I2C6, 21);
    m_test_items[MT8678_SPI_0] = new CTestItemSPI(MT8678_SPI_0, 0);
    m_test_items[MT8678_SPI_1] = new CTestItemSPI(MT8678_SPI_1, 1);
    m_test_items[MT8678_SPI_2] = new CTestItemSPI(MT8678_SPI_2, 2);
    m_test_items[MT8678_SPI_3] = new CTestItemSPI(MT8678_SPI_3, 3);
    m_test_items[MT8678_SPI_4] = new CTestItemSPI(MT8678_SPI_4, 4);
    m_test_items[MT8678_SPI_5] = new CTestItemSPI(MT8678_SPI_5, 5);
    m_test_items[MT8678_SPI_7] = new CTestItemSPI(MT8678_SPI_7, 7);
    m_test_items[MT8678_USB_2] = new CTestItemUSB(MT8678_USB_2, 2);
    m_test_items[MT8678_USB_3] = new CTestItemUSB(MT8678_USB_3, 3);
}

void CTestManage::start() {
    if (m_running) {
        return;  // 已经在运行
    }
    
    m_running = true;
    m_worker = std::thread(&CTestManage::messageLoop, this);
}

void CTestManage::pushMsg(MessageEntity* msg) {
    if (!msg) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_msgs_mutex);
        m_msgs.push(msg);
    }
    m_msg_cv.notify_one();  // 通知工作线程有新消息
}

void CTestManage::messageLoop() {
    while (m_running) {
        MessageEntity* msg = nullptr;
        
        {
            std::unique_lock<std::mutex> lock(m_msgs_mutex);
            m_msg_cv.wait(lock, [this]() {
                return !m_msgs.empty() || !m_running;
            });
            
            // 如果收到停止信号且队列为空，退出循环
            if (!m_running && m_msgs.empty()) {
                break;
            }
            
            // 取出消息
            if (!m_msgs.empty()) {
                msg = m_msgs.front();
                m_msgs.pop();
            }
        }
        
        if (msg) {
            std::thread t1([msg]() {
                
            });
            t1.detach();
        }
    }
}

void CTestManage::processMsg(MessageEntity* request) {
    CodeEntity* request_code = NULL;
    CommonEntity* response_common = nullptr;
    char* json_response = NULL;
    uint8_t* encoded = NULL;
    size_t data_len = 0;
    int FpgaCode = -1;
    int localcode = -1;
    char* printfString = NULL;

    if (request->type == 1) {
        // 本地处理测试
        request_code = CProtocol::parseCodeEntity((char*)request->data);
        if (!request_code) {
            syslog(LAYER_LOG_EER, "Failed to parse request data\n");
            goto cleanup;
        }
        syslog(LAYER_LOG_INFO, "handle request code = %d\n", request_code->code);
        localcode = request_code->code;
#ifndef BUILD_HYPERVISOR_UOS_TBOX
        if (localcode == FLY_MODE){
            //close_uart_port();
            //close_gpio();
        }
#endif
        if(m_test_items.find(localcode) == m_test_items.end()){
            response_common->state = 0;
            response_common->msg = strdup("test item not found");
        }
        response_common = m_test_items[localcode]->test();
    
        CodeEntity response_entity = {
            .code = request_code->code,
            .common = response_common
        };
        json_response = CProtocol::serializeCodeEntity(&response_entity);
        syslog(LAYER_LOG_INFO, "response json = %s \n",json_response);
        if (!json_response) {
            syslog(LAYER_LOG_EER, "JSON serialize failed\n");
            goto cleanup_code;
        }

        MessageEntity response_msg;
        response_msg.index = request->index;
        response_msg.type = request->type;
        response_msg.data = (char*)json_response;

        data_len = strlen(json_response);

        size_t encoded_len;
        encoded = CProtocol::encodeMessage(&response_msg, data_len, &encoded_len);

        printfString = CProtocol::bytes2HexStringFast((const unsigned char*)encoded,encoded_len,' ',1);
        syslog(LAYER_LOG_EER, "server send all %s\n",printfString);
        free(printfString);

        if (!encoded) {
            syslog(LAYER_LOG_EER, "Message encode failed\n");
            goto cleanup_code;
        }

        if (!m_socket_sender(encoded, encoded_len)) {
            syslog(LAYER_LOG_EER, "Send failed\n");
        }

        free(encoded);
        free(json_response);
        encoded = NULL;
        json_response = NULL;

    cleanup_code:
        if (request_code) {
            CProtocol::freeCodeEntity(request_code);
        }
        if (response_common->msg) {
            free(response_common->msg);
        }
    }
    else if (request->type == 0) {
        // 串口转发处理
        // 1. 计算数据长度
        size_t data_len = request->data_len;
        FpgaCode = request->data[0];

        syslog(LAYER_LOG_INFO, "UART request code = %d\n", FpgaCode);
        applyCmd(FpgaCode);

        // 2. 编码请求消息
        size_t encoded_len;
        encoded = CProtocol::encodeMessage(request, data_len, &encoded_len);

        if (!encoded) {
            syslog(LAYER_LOG_EER, "Message encode failed\n");
            goto cleanup;
        }

        printfString = CProtocol::bytes2HexStringFast((const unsigned char*)encoded, encoded_len,' ',1);
        syslog(LAYER_LOG_EER, "send to fpga %s\n",printfString);
        free(printfString);

        // 3. 发送到串口
        if (!m_uart_sender(encoded, encoded_len)) {
            syslog(LAYER_LOG_EER, "UART send failed\n");
            free(encoded);
            goto cleanup;
        }

        free(encoded);
        encoded = NULL;
    }

cleanup:
    if (json_response) free(json_response);
    if (encoded) free(encoded);
    CProtocol::freeMessage(request);
}

void CTestManage::applyCmd(int code) {
    int ret = -1;
    switch(code) {
        case FPGA_DSIO:
            ret = write_to_file("/proc/clkdbg","reg_write 0x32490230 0xc41\n");
            syslog(LAYER_LOG_INFO ,"DSIO write reg_write 0x32490230 0xc41 to /proc/clkdbg: %d",ret);
            run_command_exit_code("cat /proc/clkdbg");
            printf("DSIO run pattern: %d",ret);
            usleep(CMD_DELAY_US);
            break;
        case FPGA_DSI1:
            ret = write_to_file("/proc/clkdbg","reg_write 0x324A0230 0xc41\n");
            run_command_exit_code("cat /proc/clkdbg");
            syslog(LAYER_LOG_INFO ,"DSI1 write reg_write 0x324A0230 0xc41 to /proc/clkdbg: %d",ret);
            usleep(CMD_DELAY_US);
            break;
        case FPGA_DSI2:
            ret = write_to_file("/proc/clkdbg","reg_write 0x324B0230 0xc41\n");
            syslog(LAYER_LOG_INFO ,"DSI2 write reg_write 0x324B0230 0xc41 to /proc/clkdbg: %d",ret);
            run_command_exit_code("cat /proc/clkdbg");
            usleep(CMD_DELAY_US);
            break;
        case FPGA_DP:
            ret = write_to_file("/sys/kernel/debug/mtkfb","gce_wr:0x32430f00,0xc61,0xffffffff\n");
            syslog(LAYER_LOG_INFO ,"DP write gce_wr:0x32430f00,0xc61,0xffffffff to /sys/kernel/debug/mtkfb: %d",ret);
            ret =  write_to_file("/proc/mtkfb","gce_wr:0x32430f00,c61,ffffffff\n");
            syslog(LAYER_LOG_INFO ,"DP write gce_wr:0x32430f00,c61,ffffffff to /proc/mtkfb: %d",ret);
            usleep(CMD_DELAY_US);
            break;
        case FPGA_EDP:
            ret = write_to_file("/sys/kernel/debug/mtkfb","gce_wr:0x324c0100,0x141,0xffffffff\n");
            syslog(LAYER_LOG_INFO ,"EDP write gce_wr:0x324c0100,0x141,0xffffffff to /sys/kernel/debug/mtkfb: %d",ret);
            run_command_exit_code("cat /sys/kernel/debug/mtkfb");
            usleep(CMD_DELAY_US);
            break;
        case FPGA_DISP_PWM2:
            ret = write_to_file("/sys/class/leds/lcd-backlight2/brightness","127\n");
            syslog(LAYER_LOG_INFO ,"DISP_PWM2 run cmd %d",ret);
            usleep(PWM_DELAY_US);
            break;
        case FPGA_DISP_PWM3:
            ret = write_to_file("/sys/class/leds/lcd-backlight3/brightness","127\n");
            syslog(LAYER_LOG_INFO ,"DISP_PWM3 run cmd %d",ret);
            usleep(PWM_DELAY_US);
            break;
        case FPGA_PWM0:
            ret = write_to_file("/sys/devices/platform/soc/16330000.pwm/pwm_debug","0,3,1\n");
            syslog(LAYER_LOG_INFO ,"pwm0 cmd: %d",ret);
            usleep(PWM_DELAY_US);
            break;
        case FPGA_PWM3:
            ret = write_to_file("/sys/devices/platform/soc/16330000.pwm/pwm_debug","3,3,1\n");
            syslog(LAYER_LOG_INFO ,"pwm3 cmd: %d",ret);
            usleep(PWM_DELAY_US);
            break;
        default:
            break;
    }
}

int CTestManage::write_to_file(const char* file_path, const char* cmd) {
    int fd = -1;
    ssize_t bytes_written = -1;

    if (!file_path || !cmd) {
        syslog(LOG_ERR, "Invalid arguments: path=%p, cmd=%p", file_path, cmd);
        return -EINVAL;
    }

    fd = open(file_path, O_WRONLY);
    if (fd < 0) {
        int err = errno;
        syslog(LOG_ERR, "Failed to open %s: %s", file_path, strerror(err));
        return -err;
    }

    bytes_written = write(fd, cmd, strlen(cmd));
    if (bytes_written < 0) {
        int err = errno;
        close(fd);
        syslog(LOG_ERR, "Failed to write to %s: %s", file_path, strerror(err));
        return -err;
    }

    if ((size_t)bytes_written != strlen(cmd)) {
        close(fd);
        syslog(LOG_ERR, "Partial write to %s: %zd/%zu bytes",
               file_path, bytes_written, strlen(cmd));
        return -EIO;
    }

    close(fd);
    syslog(LOG_INFO, "Successfully wrote %zd bytes to %s", bytes_written, file_path);
    return 0;
}

int CTestManage::run_command_exit_code(const char* cmd) {
    int status = system(cmd);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

void CTestManage::cleanCmd(int code) { //close pattern cmd
    int ret = -1;
    switch(code) {
        case FPGA_DSIO:
            ret = write_to_file("/proc/clkdbg","reg_write 0x32490230 0x00\n");
            syslog(LAYER_LOG_INFO ,"DSIO write reg_write 0x32490230 0x00 to /proc/clkdbg: %d",ret);
            run_command_exit_code("cat /proc/clkdbg");
            break;
        case FPGA_DSI1:
            ret = write_to_file("/proc/clkdbg","reg_write 0x324A0230 0x00\n");
            run_command_exit_code("cat /proc/clkdbg");
            syslog(LAYER_LOG_INFO ,"DSI1 write reg_write 0x324A0230 0x00 to /proc/clkdbg: %d",ret);
            break;
        case FPGA_DSI2:
            ret = write_to_file("/proc/clkdbg","reg_write 0x324B0230 0x00\n");
            run_command_exit_code("cat /proc/clkdbg");
            syslog(LAYER_LOG_INFO ,"DSI2 write reg_write 0x324B0230 0x00 to /proc/clkdbg: %d",ret);
            break;
        case FPGA_DP:
            ret = write_to_file("/sys/kernel/debug/mtkfb","gce_wr:0x32430f00,0x00,0xffffffff\n");
            syslog(LAYER_LOG_INFO ,"DP write gce_wr:0x32430f00,0x00,0xffffffff to /sys/kernel/debug/mtkfb: %d",ret);
            break;
        case FPGA_EDP:
            ret = write_to_file("/sys/kernel/debug/mtkfb","gce_wr:0x324c0100,0x00,0xffffffff\n");
            // run_command_exit_code("cat /sys/kernel/debug/mtkfb");
            syslog(LAYER_LOG_INFO ,"EDP write gce_wr:0x324c0100,0x00,0xffffffff to /sys/kernel/debug/mtkfb: %d",ret);
            break;
        default:
            break;
    }
}
