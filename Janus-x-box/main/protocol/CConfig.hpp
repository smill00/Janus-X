//
// Created by LEGION on 2026/3/26.
//

#ifndef JANUS_X_BOX_CCONFIG_H
#define JANUS_X_BOX_CCONFIG_H


#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string>

enum LISTEN_MODE {
    LISTEN_SERIAL,
    LISTEN_AP,
    LISTEN_ETHERNET
};

#define AP_CONFIG_NAME "ap_config"
#define ETH_CONFIG_NAME "eth_config"

struct SSystemConfig {
    std::string name;
    std::string version;
    std::string device_id;
    std::string author;
    LISTEN_MODE mode;
    int8_t ch_def;
};

struct NetConfig {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t gateway[4];
    uint8_t netmask[4];
    uint8_t dns[4];
};

struct SUartConfig {
    int number;
    int baud_rate;
    int stop_bit;
    int parity;
};

class CConfig
{
public:
    CConfig();
    ~CConfig();

    void init();

    static void writeSystemConfig(const SSystemConfig& cfg);
    static SSystemConfig readSystemConfig();

    static bool writeNetConfig(const char* namespace_name, const NetConfig& config);
    static NetConfig readNetConfig(const char* namespace_name);

    static bool writeUartConfig(const SUartConfig& config);
    static SUartConfig readUartConfig();
};



#endif //JANUS_X_BOX_CCONFIG_H