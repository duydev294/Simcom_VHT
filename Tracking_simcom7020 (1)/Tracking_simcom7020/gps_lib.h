#ifndef GPS_RECEI_H_
#define GPS_RECEI_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event.h"

#define NUMERO_PORTA_SERIALE1 UART_NUM_1
#define U1RXD 32
#define U1TXD 33
#define BUF_SIZE (2048)
#define RD_BUF_SIZE (1024)

typedef struct 
{
    char GPS_RX_DATA[512];
    int GPS_RX_INDEX;
    bool FLAG_CHECK;
    bool FLAG_START;
    float longitude;
    float latitude;
    float utc_time;
    char ns, ew;
    float speed_k;
    float course_d;
    int date;
    char msl_units;
    uint32_t detect;
    int st_mess;
}GPS;
extern GPS gps_t;
typedef struct
{
  int hour;
  int minute;
  int second;
  char time[128];
  int day;
  int month;
  int year;
  char date[128];
}DT;
extern DT DateTime;
extern char gps_rx[256];
extern uint8_t Datetime[20];

extern bool Flag_gps_fix;
extern bool Flag_gps_acc;
extern bool Flag_receive;
extern bool Flag_connect_mqtt;
extern bool GPS_ON;

void init_GPS(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud_rate);
static void UART_ISR_GPS(void *pvParameters);
static void GPS_RX_Process();
int filter_comma(char *respond_data, int begin, int end, char *output);
static void Conver_DateTime(char *datetime, char kind);
time_t string_to_seconds(const char *timestamp_str);
#endif /* GPS_RECEI_H_ */
