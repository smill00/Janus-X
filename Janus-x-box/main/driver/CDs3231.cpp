//
// Created by LEGION on 2026/3/27.
//

#include "cds3231.h"
#include "esp_log.h"

static const char* TAG = "DS3231";

// 私有构造函数
CDs3231::CDs3231(i2c_port_t port, uint8_t addr)
    : i2c_port(port), device_addr(addr) {
    ESP_LOGI(TAG, "DS3231 instance created with I2C port %d, address 0x%02X", port, addr);
}

// 获取单例实例
CDs3231& CDs3231::ins(i2c_port_t port, uint8_t addr) {
    static CDs3231 instance(port, addr);
    return instance;
}

// 初始化I2C
esp_err_t CDs3231::init(int sda_pin, int scl_pin, uint32_t clk_speed) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {.clk_speed = clk_speed},
    };

    esp_err_t err = i2c_param_config(i2c_port, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C parameter config failed: 0x%X", err);
        return err;
    }

    err = i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: 0x%X", err);
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized on SDA:%d, SCL:%d, speed:%luHz", sda_pin, scl_pin, clk_speed);
    return ESP_OK;
}

// 写寄存器
esp_err_t CDs3231::write_register(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write register 0x%02X=0x%02X failed: 0x%X", reg, data, ret);
    }

    return ret;
}

// 读取多个寄存器
esp_err_t CDs3231::read_registers(uint8_t start_reg, uint8_t* buffer, uint8_t length) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, start_reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);

    if (length > 1) {
        i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, buffer + length - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read %d registers from 0x%02X failed: 0x%X", length, start_reg, ret);
    }

    return ret;
}

// BCD转十进制
uint8_t CDs3231::bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 十进制转BCD
uint8_t CDs3231::dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// 设置时间
esp_err_t CDs3231::setTime(const ds3231_time_t* time) {
    uint8_t data[7];

    data[0] = dec_to_bcd(time->seconds);
    data[1] = dec_to_bcd(time->minutes);
    data[2] = dec_to_bcd(time->hours); // 24小时模式
    data[3] = dec_to_bcd(time->day);
    data[4] = dec_to_bcd(time->date);
    data[5] = dec_to_bcd(time->month);
    data[6] = dec_to_bcd(time->year);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, DS3231_REG_SECONDS, true);

    for (int i = 0; i < 7; i++) {
        i2c_master_write_byte(cmd, data[i], true);
    }

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Time set: 20%02d-%02d-%02d %02d:%02d:%02d",
                time->year, time->month, time->date,
                time->hours, time->minutes, time->seconds);
    }

    return ret;
}

// 设置时间（参数格式）
esp_err_t CDs3231::setTime(uint8_t year, uint8_t month, uint8_t date,
                          uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day) {
    ds3231_time_t time = {
        .seconds = seconds,
        .minutes = minutes,
        .hours = hours,
        .day = day,
        .date = date,
        .month = month,
        .year = year
    };

    return setTime(&time);
}

// 读取时间
esp_err_t CDs3231::get_time(ds3231_time_t* time) {
    uint8_t data[7];

    esp_err_t ret = read_registers(DS3231_REG_SECONDS, data, 7);
    if (ret != ESP_OK) {
        return ret;
    }

    time->seconds = bcd_to_dec(data[0] & 0x7F);
    time->minutes = bcd_to_dec(data[1] & 0x7F);

    // 处理小时（24小时模式）
    uint8_t hour_reg = data[2];
    if (hour_reg & 0x40) {
        // 12小时模式
        time->hours = bcd_to_dec(hour_reg & 0x1F);
        if (hour_reg & 0x20) {
            time->hours += 12; // PM
        }
    } else {
        // 24小时模式
        time->hours = bcd_to_dec(hour_reg & 0x3F);
    }

    time->day = bcd_to_dec(data[3] & 0x07);
    time->date = bcd_to_dec(data[4] & 0x3F);
    time->month = bcd_to_dec(data[5] & 0x1F);
    time->year = bcd_to_dec(data[6]);

    return ESP_OK;
}

// 简化函数：直接设置当前时间
esp_err_t CDs3231::set_now(uint8_t year, uint8_t month, uint8_t day,
                          uint8_t hour, uint8_t min, uint8_t sec) {
    return setTime(year, month, day, hour, min, sec, 1);
}