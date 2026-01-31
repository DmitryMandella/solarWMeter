

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

void app_main(void)
{
    // 1️⃣ Конфигурация UART
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT  // ✅ ДОБАВЛЕНО!
    };

    // ✅ ПРАВИЛЬНЫЙ ПОРЯДОК:
    // 1. Сначала настроить параметры
    uart_param_config(SIM800_UART, &uart_config);
    
    // 2. Потом назначить пины
    uart_set_pin(SIM800_UART, SIM800_TX, SIM800_RX,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    // 3. В конце установить драйвер
    uart_driver_install(SIM800_UART, BUF_SIZE * 2, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART initialized");

    // ✅ ВАЖНО: SIM800C нужно время на загрузку!
    vTaskDelay(pdMS_TO_TICKS(5000)); // 5 секунд вместо 1!

    // 2️⃣ Отправляем AT
    const char *cmd = "AT\r\n";
    uart_write_bytes(SIM800_UART, cmd, strlen(cmd));
    ESP_LOGI(TAG, ">> AT");

    // 3️⃣ Читаем ответ
    uint8_t data[BUF_SIZE];
    while (1)
    {
        int len = uart_read_bytes(
            SIM800_UART,
            data,
            BUF_SIZE - 1,
            pdMS_TO_TICKS(1000)
        );

        if (len > 0)
        {
            data[len] = '\0';
            ESP_LOGI(TAG, "<< %s", (char *)data);
        }
        else
        {
            // ✅ ДОБАВЛЕНО: Повторная отправка если нет ответа
            ESP_LOGW(TAG, "No response, retrying...");
            uart_write_bytes(SIM800_UART, cmd, strlen(cmd));
            ESP_LOGI(TAG, ">> AT");
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // небольшая задержка
    }
}