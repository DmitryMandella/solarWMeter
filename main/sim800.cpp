#include "sim800.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

static uart_port_t sim800_uart = UART_NUM_MAX; // сюда сохраним переданный UART

void sim800_set_uart(uart_port_t uart) {
    sim800_uart = uart;
}

void send_at_command(const char *cmd) {
    if (sim800_uart == UART_NUM_MAX) return; // UART не установлен
    uart_write_bytes(sim800_uart, cmd, strlen(cmd));
    uart_write_bytes(sim800_uart, "\r\n", 2);
}

// Чтение UART с таймаутом молчания
int uart_read_with_wait( int uart_num,  // номер UART
    char *buf,             // указатель на буфер
    int buf_size,          // размер буфера
    int silence_timeout_ms // таймаут
) {
    int total = 0;
    int64_t last_rx = esp_timer_get_time();

    while (1) {
        int len = uart_read_bytes(
            (uart_port_t)uart_num,
            (uint8_t *)&buf[total],
            buf_size - total - 1,
            pdMS_TO_TICKS(50)
        );

        if (len > 0) {
            total += len;
            buf[total] = '\0';
            last_rx = esp_timer_get_time();

            if (total >= buf_size - 1)
                break;
        } else {
            if ((esp_timer_get_time() - last_rx) > silence_timeout_ms * 1000)
                break;
        }
    }

    return total;
}

bool check_SIM800() {
    if (sim800_uart == UART_NUM_MAX) return false; // UART не установлен

    send_at_command("AT");

    char buf[128];
    int buf_size = sizeof(buf);
    int silence_timeout_ms = 1000; // 1 секунда
    int len = uart_read_with_wait(sim800_uart, buf, buf_size, silence_timeout_ms);

    // Проверяем, пришёл ли ответ "OK"
    if (len > 0 && strstr(buf, "OK") != NULL) {
        return true;
    } else {
        return false;
    }
}

bool send_to_thingspeak(const char* field_value) {
    if (sim800_uart == UART_NUM_MAX || !field_value) {
        printf("UART не установлен или значение пустое.\n");
        return false;
    }

    char buf[512];

    printf("==> Настройка GPRS...\n");
    send_at_command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 2000);
    printf("Ответ SIM800: %s\n", buf);

    snprintf(buf, sizeof(buf), "AT+SAPBR=3,1,\"APN\",\"www.kyivstar.net\"");
    send_at_command(buf);
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 2000);
    printf("APN установлен, ответ: %s\n", buf);

    printf("==> Открытие GPRS контекста...\n");
    send_at_command("AT+SAPBR=1,1");
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 5000);
    printf("Ответ на открытие GPRS: %s\n", buf);

    printf("==> Инициализация HTTP...\n");
    send_at_command("AT+HTTPINIT");
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 2000);
    printf("HTTPINIT ответ: %s\n", buf);

    send_at_command("AT+HTTPPARA=\"CID\",1");
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 2000);
    printf("HTTPPARA CID ответ: %s\n", buf);

    snprintf(buf, sizeof(buf),
        "AT+HTTPPARA=\"URL\",\"https://api.thingspeak.com/update?api_key=R50KLV45YPIOG0N2&field1=%s\"",
        field_value
    );
    printf("==> Устанавливаем URL: %s\n", buf);
    send_at_command(buf);
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 2000);
    printf("HTTPPARA URL ответ: %s\n", buf);

    printf("==> Отправка GET-запроса...\n");
    send_at_command("AT+HTTPACTION=0");

    // Ждём ответа с кодом HTTP
    int len = uart_read_with_wait(sim800_uart, buf, sizeof(buf), 10000); // таймаут 10 сек
    printf("HTTPACTION ответ:\n%s\n", buf);

    // Теперь читаем тело ответа сервера
    printf("==> Чтение ответа HTTP...\n");
    send_at_command("AT+HTTPREAD");
    len = uart_read_with_wait(sim800_uart, buf, sizeof(buf), 5000);
    printf("HTTPREAD ответ:\n%s\n", buf);

    printf("==> Завершение HTTP...\n");
    send_at_command("AT+HTTPTERM");
    uart_read_with_wait(sim800_uart, buf, sizeof(buf), 2000);
    printf("HTTPTERM ответ: %s\n", buf);

    // Проверяем наличие HTTP-кода 200 в ответе от сервера
    if (strstr(buf, "200") != NULL) {
        printf("Данные успешно отправлены на ThingSpeak.\n");
        return true;
    } else {
        printf("Ошибка при отправке данных или HTTP-код != 200: %s\n", buf);
        return false;
    }
}
