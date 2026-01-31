#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TAG "SIM800"

// UART для SIM800C
#define SIM800_UART UART_NUM_1
#define SIM800_TX   18
#define SIM800_RX   17
#define BUF_SIZE    256

void send_at_command(const char *cmd)
{
    uart_write_bytes(SIM800_UART, cmd, strlen(cmd));
    ESP_LOGI(TAG, ">> %s", cmd);
}

void app_main(void)
{
    // 1️⃣ Конфигурация UART
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

    // Даем SIM800 время на загрузку
    vTaskDelay(pdMS_TO_TICKS(5000));

    uint8_t data[BUF_SIZE];

    // 2️⃣ Отключаем эхо
    send_at_command("ATE0\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    int len = uart_read_bytes(SIM800_UART, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0)
    {
        data[len] = '\0';
        ESP_LOGI(TAG, "<< %s", data);
    }

    // 3️⃣ Проверка связи AT
    const char *cmd = "AT\r\n";
    while (1)
    {
        send_at_command(cmd);

        len = uart_read_bytes(SIM800_UART, data, BUF_SIZE - 1, pdMS_TO_TICKS(1000));
        if (len > 0)
        {
            data[len] = '\0';
            ESP_LOGI(TAG, "<< %s", data);

            // Проверяем наличие OK
            if (strstr((char *)data, "OK") != NULL)
            {
                ESP_LOGI(TAG, "Command executed successfully");
            }
            else if (strstr((char *)data, "ERROR") != NULL)
            {
                ESP_LOGW(TAG, "SIM800 returned ERROR");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // задержка между командами
    }
}
