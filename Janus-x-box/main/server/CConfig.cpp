//
// Created by LEGION on 2026/3/26.
//

#include "CConfig.hpp"

static const char* TAG = "SystemConfig";

CConfig::CConfig()
{
}

CConfig::~CConfig()
{
}

void CConfig::init()
{
    nvs_handle_t handle;
    uint8_t reset_value = 0;

    // 打开NVS
    if (nvs_open("system", NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGE(TAG, "无法打开NVS命名空间");
        return;
    }

    // 尝试读取
    esp_err_t err = nvs_get_u8(handle, "reset", &reset_value);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // 不存在则创建
        reset_value = 1;
        nvs_set_u8(handle, "reset", reset_value);
        nvs_commit(handle);
        ESP_LOGI(TAG, "创建reset键，值: %d", reset_value);
    }
    else if (err == ESP_OK) {
        // 存在则打印
        ESP_LOGI(TAG, "当前reset值: %d", reset_value);

        // 简单判断
        switch (reset_value) {
        case 0:  ESP_LOGI(TAG, "状态: 正常"); break;
        case 1:  ESP_LOGI(TAG, "状态: 复位"); break;
        case 2:  ESP_LOGI(TAG, "状态: 待恢复"); break;
        default: ESP_LOGI(TAG, "状态: 自定义(%d)", reset_value); break;
        }
    }

    nvs_close(handle);
}

void CConfig::writeSystemConfig(const SSystemConfig& cfg) {
    nvs_handle_t handle;

    // 1. 打开system命名空间
    if (nvs_open("system", NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGE("Config", "无法打开system命名空间");
        return;
    }

    esp_err_t err = ESP_OK;
    bool all_success = true;

    // 2. 存储字符串字段
    if (!cfg.name.empty()) {
        err = nvs_set_str(handle, "sys_name", cfg.name.c_str());
        if (err != ESP_OK) {
            ESP_LOGE("Config", "存储name失败: %s", esp_err_to_name(err));
            all_success = false;
        }
    }

    if (!cfg.version.empty()) {
        err = nvs_set_str(handle, "sys_ver", cfg.version.c_str());
        if (err != ESP_OK) {
            ESP_LOGE("Config", "存储version失败: %s", esp_err_to_name(err));
            all_success = false;
        }
    }

    if (!cfg.device_id.empty()) {
        err = nvs_set_str(handle, "dev_id", cfg.device_id.c_str());
        if (err != ESP_OK) {
            ESP_LOGE("Config", "存储device_id失败: %s", esp_err_to_name(err));
            all_success = false;
        }
    }

    if (!cfg.author.empty()) {
        err = nvs_set_str(handle, "author", cfg.author.c_str());
        if (err != ESP_OK) {
            ESP_LOGE("Config", "存储author失败: %s", esp_err_to_name(err));
            all_success = false;
        }
    }

    // 3. 存储整数字段
    err = nvs_set_i32(handle, "mode", cfg.mode);
    if (err != ESP_OK) {
        ESP_LOGE("Config", "存储mode失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    err = nvs_set_i32(handle, "ch_def", cfg.ch_def);
    if (err != ESP_OK) {
        ESP_LOGE("Config", "存储ch_def失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // 4. 设置一个标记表示配置已存储
    uint8_t config_stored = 1;
    err = nvs_set_u8(handle, "config_stored", config_stored);
    if (err != ESP_OK) {
        ESP_LOGW("Config", "设置配置标记失败: %s", esp_err_to_name(err));
    }

    // 5. 提交更改
    if (all_success) {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE("Config", "提交配置失败: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI("Config", "系统配置保存成功");
        }
    } else {
        ESP_LOGW("Config", "部分配置保存失败");
    }

    // 6. 关闭句柄
    nvs_close(handle);
}

SSystemConfig CConfig::readSystemConfig() {
    SSystemConfig cfg;
    nvs_handle_t handle;

    // 1. 打开system命名空间
    if (nvs_open("system", NVS_READONLY, &handle) != ESP_OK) {
        ESP_LOGE("Config", "无法打开system命名空间");
        return cfg; // 返回默认配置
    }

    // 2. 检查配置是否存在
    uint8_t config_stored = 0;
    esp_err_t err = nvs_get_u8(handle, "config_stored", &config_stored);
    if (err != ESP_OK || config_stored == 0) {
        ESP_LOGI("Config", "没有找到已存储的系统配置");
        nvs_close(handle);
        return cfg; // 返回默认配置
    }

    // 3. 读取字符串字段
    size_t required_size = 0;

    // 读取name
    err = nvs_get_str(handle, "sys_name", NULL, &required_size);
    if (err == ESP_OK) {
        char* buffer = new char[required_size];
        err = nvs_get_str(handle, "sys_name", buffer, &required_size);
        if (err == ESP_OK) {
            cfg.name = buffer;
        }
        delete[] buffer;
    }

    // 读取version
    err = nvs_get_str(handle, "sys_ver", NULL, &required_size);
    if (err == ESP_OK) {
        char* buffer = new char[required_size];
        err = nvs_get_str(handle, "sys_ver", buffer, &required_size);
        if (err == ESP_OK) {
            cfg.version = buffer;
        }
        delete[] buffer;
    }

    // 读取device_id
    err = nvs_get_str(handle, "dev_id", NULL, &required_size);
    if (err == ESP_OK) {
        char* buffer = new char[required_size];
        err = nvs_get_str(handle, "dev_id", buffer, &required_size);
        if (err == ESP_OK) {
            cfg.device_id = buffer;
        }
        delete[] buffer;
    }

    // 读取author
    err = nvs_get_str(handle, "author", NULL, &required_size);
    if (err == ESP_OK) {
        char* buffer = new char[required_size];
        err = nvs_get_str(handle, "author", buffer, &required_size);
        if (err == ESP_OK) {
            cfg.author = buffer;
        }
        delete[] buffer;
    }

    // 4. 读取整数字段
    int8_t temp = 0;

    err = nvs_get_i8(handle, "mode", &temp);
    if (err == ESP_OK) {
        cfg.mode = static_cast<LISTEN_MODE>(temp);
    }

    err = nvs_get_i8(handle, "ch_def", &temp);
    if (err == ESP_OK) {
        cfg.ch_def = temp;
    }

    nvs_close(handle);
    return cfg;
}

// 写入NetConfig到指定命名空间
bool CConfig::writeNetConfig(const char* namespace_name, const NetConfig& config) {
    nvs_handle_t handle;
    esp_err_t err;
    bool all_success = true;

    // 1. 打开指定命名空间
    err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "无法打开命名空间 '%s': %s", namespace_name, esp_err_to_name(err));
        return false;
    }

    // 2. 存储各个字段
    // MAC地址
    err = nvs_set_blob(handle, "mac", config.mac, sizeof(config.mac));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储mac失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // IP地址
    err = nvs_set_blob(handle, "ip", config.ip, sizeof(config.ip));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储ip失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // 网关
    err = nvs_set_blob(handle, "gateway", config.gateway, sizeof(config.gateway));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储gateway失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // 子网掩码
    err = nvs_set_blob(handle, "netmask", config.netmask, sizeof(config.netmask));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储netmask失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // DNS
    err = nvs_set_blob(handle, "dns", config.dns, sizeof(config.dns));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储dns失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // 3. 设置配置版本标记
    uint8_t version = 1;
    err = nvs_set_u8(handle, "config_version", version);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "存储版本标记失败: %s", esp_err_to_name(err));
    }

    // 4. 设置配置类型标记
    err = nvs_set_str(handle, "config_type", "NetConfig");
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "存储配置类型标记失败: %s", esp_err_to_name(err));
    }

    // 5. 提交更改
    if (all_success) {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "提交配置失败: %s", esp_err_to_name(err));
            all_success = false;
        } else {
            ESP_LOGI(TAG, "NetConfig保存到命名空间 '%s' 成功", namespace_name);
        }
    } else {
        ESP_LOGW(TAG, "NetConfig保存到命名空间 '%s' 部分失败", namespace_name);
    }

    // 6. 关闭句柄
    nvs_close(handle);

    return all_success;
}

// 从指定命名空间读取NetConfig
NetConfig CConfig::readNetConfig(const char* namespace_name) {
    NetConfig config;  // 先获取默认配置
    nvs_handle_t handle;
    esp_err_t err;
    size_t length;

    // 1. 打开指定命名空间
    err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "无法打开命名空间 '%s': %s", namespace_name, esp_err_to_name(err));
        return config;  // 返回默认配置
    }

    // 2. 检查配置类型
    char config_type[32];
    size_t config_type_len = sizeof(config_type);
    err = nvs_get_str(handle, "config_type", config_type, &config_type_len);
    if (err != ESP_OK || strcmp(config_type, "NetConfig") != 0) {
        ESP_LOGW(TAG, "命名空间 '%s' 中的配置类型不匹配或不存在", namespace_name);
        nvs_close(handle);
        return config;  // 返回默认配置
    }

    // 3. 读取各个字段
    bool all_success = true;

    // 读取MAC地址
    length = sizeof(config.mac);
    err = nvs_get_blob(handle, "mac", config.mac, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取mac失败: %s", esp_err_to_name(err));
        all_success = false;
    } else if (length != sizeof(config.mac)) {
        ESP_LOGE(TAG, "mac长度不匹配: 期望%zu, 实际%zu", sizeof(config.mac), length);
        all_success = false;
    }

    // 读取IP地址
    length = sizeof(config.ip);
    err = nvs_get_blob(handle, "ip", config.ip, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取ip失败: %s", esp_err_to_name(err));
        all_success = false;
    } else if (length != sizeof(config.ip)) {
        ESP_LOGE(TAG, "ip长度不匹配: 期望%zu, 实际%zu", sizeof(config.ip), length);
        all_success = false;
    }

    // 读取网关
    length = sizeof(config.gateway);
    err = nvs_get_blob(handle, "gateway", config.gateway, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取gateway失败: %s", esp_err_to_name(err));
        all_success = false;
    } else if (length != sizeof(config.gateway)) {
        ESP_LOGE(TAG, "gateway长度不匹配: 期望%zu, 实际%zu", sizeof(config.gateway), length);
        all_success = false;
    }

    // 读取子网掩码
    length = sizeof(config.netmask);
    err = nvs_get_blob(handle, "netmask", config.netmask, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取netmask失败: %s", esp_err_to_name(err));
        all_success = false;
    } else if (length != sizeof(config.netmask)) {
        ESP_LOGE(TAG, "netmask长度不匹配: 期望%zu, 实际%zu", sizeof(config.netmask), length);
        all_success = false;
    }

    // 读取DNS
    length = sizeof(config.dns);
    err = nvs_get_blob(handle, "dns", config.dns, &length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取dns失败: %s", esp_err_to_name(err));
        all_success = false;
    } else if (length != sizeof(config.dns)) {
        ESP_LOGE(TAG, "dns长度不匹配: 期望%zu, 实际%zu", sizeof(config.dns), length);
        all_success = false;
    }

    // 4. 关闭句柄
    nvs_close(handle);

    if (all_success) {
        ESP_LOGI(TAG, "从命名空间 '%s' 读取NetConfig成功", namespace_name);
    } else {
        ESP_LOGW(TAG, "从命名空间 '%s' 读取NetConfig不完整", namespace_name);
    }

    return config;
}

// 写入SUartConfig到指定命名空间
bool CConfig::writeUartConfig( const SUartConfig& config) {
    const char* namespace_name = "UartCfg";
    nvs_handle_t handle;
    esp_err_t err;
    bool all_success = true;

    // 1. 打开指定命名空间
    err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "无法打开命名空间 '%s': %s", namespace_name, esp_err_to_name(err));
        return false;
    }

    // 2. 存储整数字段
    err = nvs_set_i32(handle, "uart_number", config.number);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储 uart_number 失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    err = nvs_set_i32(handle, "uart_baud", config.baud_rate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储 uart_baud 失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    err = nvs_set_i32(handle, "uart_stopbit", config.stop_bit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储 uart_stopbit 失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    err = nvs_set_i32(handle, "uart_parity", config.parity);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "存储 uart_parity 失败: %s", esp_err_to_name(err));
        all_success = false;
    }

    // 4. 设置配置类型标记，便于读取时识别
    err = nvs_set_str(handle, "config_type", "UartConfig");
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "存储配置类型标记失败: %s", esp_err_to_name(err));
    }

    // 5. 提交更改
    if (all_success) {
        err = nvs_commit(handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "提交UartConfig失败: %s", esp_err_to_name(err));
            all_success = false;
        } else {
            ESP_LOGI(TAG, "SUartConfig保存到命名空间 '%s' 成功", namespace_name);
        }
    } else {
        ESP_LOGW(TAG, "SUartConfig保存到命名空间 '%s' 部分失败", namespace_name);
    }

    // 6. 关闭句柄
    nvs_close(handle);
    return all_success;
}

// 从指定命名空间读取SUartConfig
SUartConfig CConfig::readUartConfig() {
    const char* namespace_name = "UartCfg";
    SUartConfig config{}; // 使用结构体的默认构造函数初始化
    nvs_handle_t handle;
    esp_err_t err;

    // 1. 打开指定命名空间
    err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "无法打开命名空间 '%s': %s", namespace_name, esp_err_to_name(err));
        return config;  // 返回默认配置
    }

    // 2. 检查配置类型 (可选，但推荐用于区分配置)
    char config_type[32];
    size_t config_type_len = sizeof(config_type);
    err = nvs_get_str(handle, "config_type", config_type, &config_type_len);
    if (err == ESP_OK && strcmp(config_type, "UartConfig") != 0) {
        ESP_LOGW(TAG, "命名空间 '%s' 中的配置类型不匹配", namespace_name);
    }

    // 3. 读取整数字段
    int32_t temp_val = 0;
    err = nvs_get_i32(handle, "uart_number", &temp_val);
    if (err == ESP_OK) {
        config.number = temp_val;
    }

    err = nvs_get_i32(handle, "uart_baud", &temp_val);
    if (err == ESP_OK) {
        config.baud_rate = temp_val;
    }

    err = nvs_get_i32(handle, "uart_stopbit", &temp_val);
    if (err == ESP_OK) {
        config.stop_bit = temp_val;
    }

    err = nvs_get_i32(handle, "uart_parity", &temp_val);
    if (err == ESP_OK) {
        config.parity = temp_val;
    }

    // 5. 关闭句柄
    nvs_close(handle);

    ESP_LOGI(TAG, "从命名空间 '%s' 读取UartConfig完成", namespace_name);
    return config;
}