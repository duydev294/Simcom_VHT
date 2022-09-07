#ifndef SIMCOM7020_SIMCOM7020_H_
#define SIMCOM7020_SIMCOM7020_H_

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

#define NUMERO_PORTA_SERIALE2 UART_NUM_2
#define BUF_SIZE (1024 * 2)
#define RD_BUF_SIZE (1024)     

 typedef struct simcom_t
{
 uart_port_t uart_num;
  int tx_io_num;
  int rx_io_num;
  int baud_rate;
  int timestamp;
  int tp;
  bool AT_buff_avai;
  uint8_t AT_buff[BUF_SIZE];
  void (*mqtt_CB)(char * data);
}simcom;
extern uint8_t tp_mess[10];
extern simcom simcom_7020;
#define U2RXD 16
#define U2TXD 17
 typedef enum
{
 AT_OK,
  AT_ERROR,
  AT_TIMEOUT,
}AT_flag;

typedef struct client_t
{
   char uri[51];
   int port;
   char user_name[50];
   char client_id[50];
   char password[50];
   int mqtt_id;
}client;

void init_simcom(uart_port_t uart_num, int tx_num, int rx_num, int baud_rate);
void UART_ISR_ROUTINE(void *pvParameters);
// kiem tra ket noi mang
bool isRegistered(int retry);
// kiem tra giao tiep AT command 
bool isInit(int retry);
//khoi tao ket noi mqtt
bool mqtt_start(client clientMQTT, int versionMQTT, int keepalive, int clean_session, int retry);
// ngat ket noi mqtt
bool mqtt_stop(client clientMQTT, int retry);
// dang ky topic de nhan ban tin dieu khien
bool mqtt_subscribe(client clientMQTT, char *topic, int qos, int retry,  void (*mqttSubcribeCB)(char * data));
// gui ban tin vao topic
bool mqtt_message_publish(client clientMQTT, char *data, char *topic,int qos,  int retry);

void restart_simcom();
bool isconverhex(int retry);
bool get_signal_strength(char *rssi, char *rsrp, char *rsrq, int retry);
bool getCellId(int * mcc, int * mnc, char * lac, char * cid, int retry);
int filter_comma_t(char *respond_data, int begin, int end, char *output);
#endif /* SIMCOM7020_SIMCOM7020_H_ */
