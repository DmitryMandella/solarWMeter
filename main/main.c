#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "sim800.h"

#define TAG "SIM800"

// UART для SIM800C
#define SIM800_UART UART_NUM_1
#define SIM800_TX   18
#define SIM800_RX   17
#define BUF_SIZE    256

void app_main(void)
{
    // Конфигурация UART
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    uart_param_config(SIM800_UART, &uart_config);
    uart_set_pin(SIM800_UART, SIM800_TX, SIM800_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(SIM800_UART, BUF_SIZE * 2, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART initialized");

    // Передаем UART в модуль SIM800
    sim800_set_uart(SIM800_UART);

    // Даем SIM800 время на загрузку
    vTaskDelay(pdMS_TO_TICKS(1000));

    char buf[BUF_SIZE];
    while(1){
    // Пример отправки команды
    send_at_command("ATE0");  // отключаем эхо

    // Чтение ответа (без проверки)
    int len = uart_read_with_wait(SIM800_UART, buf, BUF_SIZE, 1000);
    if (len > 0)
    {
        ESP_LOGI(TAG, "<< %s", buf);  // просто выводим то, что пришло
    }

    // Можно отправлять еще команды
    send_at_command("AT+CSQ");  // пример запроса уровня сигнала
    len = uart_read_with_wait(SIM800_UART, buf, BUF_SIZE, 1000);
    if (len > 0)
    {
        ESP_LOGI(TAG, "<< %s", buf);
    }

    if(check_SIM800()) {
        ESP_LOGI(TAG, "SIM800 is responsive");
    } else {
        ESP_LOGW(TAG, "No response from SIM800");
    }
    
    send_to_thingspeak("123"); // пример отправки данных на ThingSpeak

    
}
}
