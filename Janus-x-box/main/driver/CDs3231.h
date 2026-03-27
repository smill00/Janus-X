//
// Created by LEGION on 2026/3/27.
//

#ifndef JANUS_X_BOX_CDS3231_H
#define JANUS_X_BOX_CDS3231_H

#include "driver/i2c.h"

#define DS3231_ADDR 0x68

// DS3231寄存器地址
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_MINUTES 0x01
#define DS3231_REG_HOURS   0x02
#define DS3231_REG_DAY     0x03
#define DS3231_REG_DATE    0x04
#define DS3231_REG_MONTH   0x05
#define DS3231_REG_YEAR    0x06

// 时间结构体
typedef struct {
    uint8_t seconds;  // 0-59
    uint8_t minutes;  // 0-59
    uint8_t hours;    // 0-23 (24小时制)
    uint8_t day;      // 1-7 (星期几，1=星期日)
    uint8_t date;     // 1-31
    uint8_t month;    // 1-12
    uint8_t year;     // 0-99 (表示2000-2099)
} ds3231_time_t;

class CDs3231 {
private:
    i2c_port_t i2c_port;
    uint8_t device_addr;

    // 单例相关
    CDs3231(i2c_port_t port, uint8_t addr);

    // 禁止拷贝和赋值
    CDs3231(const CDs3231&) = delete;
    CDs3231& operator=(const CDs3231&) = delete;

    // I2C通信方法
    esp_err_t write_register(uint8_t reg, uint8_t data);
    esp_err_t read_registers(uint8_t start_reg, uint8_t* buffer, uint8_t length);

    // BCD转换
    uint8_t bcd_to_dec(uint8_t bcd);
    uint8_t dec_to_bcd(uint8_t dec);

public:
    // 获取单例实例
    static CDs3231& ins(i2c_port_t port = I2C_NUM_0, uint8_t addr = DS3231_ADDR);

    // 初始化I2C
    esp_err_t init(int sda_pin, int scl_pin, uint32_t clk_speed = 100000);

    // 设置时间
    esp_err_t setTime(const ds3231_time_t* time);
    esp_err_t set_time(uint8_t year, uint8_t month, uint8_t date,
                       uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t day = 1);

    // 读取时间
    esp_err_t get_time(ds3231_time_t* time);

    // 简化函数：直接设置当前时间
    esp_err_t set_now(uint8_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t min, uint8_t sec);

    // 获取I2C端口
    i2c_port_t get_i2c_port() const { return i2c_port; }

    // 获取设备地址
    uint8_t get_device_addr() const { return device_addr; }
};

#endif // JANUS_X_BOX_CDS3231_H