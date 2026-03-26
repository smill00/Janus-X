#include <stdio.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

class Test {
    public:
        Test() {
            std::cout << "Test constructor" << std::endl;
        }

        void print() {

            int count = 0;
            while(true) {
                printf("Hello esp32! This is ESP32CPP+ #%d\n", ++count);
                std::cout << "CPP hello world" << std::endl;
                vTaskDelay(1000 / portTICK_PERIOD_MS); // 延时1秒
            }
        }
};

extern "C" void app_main(void)
{
    Test test;
    test.print();

    return;
}
