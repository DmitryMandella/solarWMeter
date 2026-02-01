#ifndef SIM800_H
#define SIM800_H

#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

void sim800_set_uart(uart_port_t uart);

void send_at_command(const char *cmd);

int uart_read_with_wait(
    int uart_num,
    char *buf,
    int buf_size,
    int silence_timeout_ms 
);

bool check_SIM800();

bool send_to_thingspeak(const char* field_value);




#ifdef __cplusplus
}
#endif

#endif
