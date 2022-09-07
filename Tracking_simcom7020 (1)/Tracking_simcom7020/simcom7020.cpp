#include "simcom7020.h"
#include "Arduino.h"

static const char * TAG = "";    
void init_simcom(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud_rate)
{
  uart_config_t uart_config =
  {
    .baud_rate = baud_rate,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };
  int intr_alloc_flags = 0;
  uart_param_config(uart_num, &uart_config);
  uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags);
  uart_set_pin(uart_num, tx_io_num, rx_io_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  xTaskCreate(UART_ISR_ROUTINE, "uart_echo_task1", 4096, NULL, 10, NULL);
}
void UART_ISR_ROUTINE(void *pvParameters){
  uint8_t data[512];
  while (1) {
    int len = uart_read_bytes(NUMERO_PORTA_SERIALE2, data, (512 - 1), 100 / portTICK_PERIOD_MS);
    // Write data back to the UART
    if (len) {
      data[len] = '\0';
      ESP_LOGI(TAG, "Rec: %s", (char*) data);
      if(strstr((char*)data, "+CMQPUB:"))
      {
        simcom_7020.mqtt_CB((char*)data);
      }
      else if(strstr((char*)data, "+CMQPUB:")) {}
      else
      {
        memcpy(simcom_7020.AT_buff, data, len);
        simcom_7020.AT_buff_avai = true;
      }
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}
// // use UART RX send AT command
static int filter_comma(char *respond_data, int begin, int end, char *output)
{
    memset(output, 0, strlen(output));
    int count_filter = 0;
    int lim = 0;
    int start = 0;
    int finish = 0;
    int i = 0;
    for (i = 0; i < strlen(respond_data); i++)
    {
        if ( respond_data[i] == ',')
        {
            count_filter ++;
            if (count_filter == begin)          start = i+1;
            if (count_filter == end)            finish = i;
        }

    }
    lim = finish - start;
    for (i = 0; i < lim; i++){
        output[i] = respond_data[start];
        start ++;
    }
    output[i] = 0;
    return 0;
}
static void send_ATComand(char *ATcommand) {
  ESP_LOGI(TAG, "Send: %s", ATcommand);
  simcom_7020.AT_buff_avai = false;
  memset(simcom_7020.AT_buff, 0, BUF_SIZE);
  uart_write_bytes(UART_NUM_2, (char *)ATcommand, strlen((char *) ATcommand));
  uart_write_bytes(UART_NUM_2, "\r\n", strlen("\r\n"));
  vTaskDelay(100/portTICK_PERIOD_MS);
}
void restart_simcom()
{
   send_ATComand("AT+CFUN=0");
   delay(1000);
   send_ATComand("AT+CFUN=1");
}
AT_flag _readFeedback(uint32_t timeout, char *expect) {
  uint64_t timeCurrent = esp_timer_get_time() / 1000;
  while(esp_timer_get_time() / 1000 < (timeout + timeCurrent)) {
    delay(10);
    if(simcom_7020.AT_buff_avai) {
      if(strstr((char *)simcom_7020.AT_buff, "ERROR")) {delay(1000); return AT_ERROR;}
      else if(strstr((char *)simcom_7020.AT_buff, expect)) return AT_OK;
    }
  }
  return AT_TIMEOUT;
}
bool _readSerial(uint32_t timeout)
{
  uint64_t timeOld = esp_timer_get_time() / 1000;
  while (!simcom_7020.AT_buff_avai && !(esp_timer_get_time() / 1000 > timeOld + timeout))
  {
    vTaskDelay(10/portTICK_PERIOD_MS);
  }
  if(simcom_7020.AT_buff_avai == false) return false;
  else return true;
}
bool isRegistered(int retry)
{
  while(retry--)
  {
    vTaskDelay(1000/portTICK_PERIOD_MS);
    send_ATComand("AT+CREG?");
    if(_readSerial(1000) == false) continue;
    if(strstr((char*)simcom_7020.AT_buff, "0,1") || strstr((char*)simcom_7020.AT_buff, "0,5") || strstr((char*)simcom_7020.AT_buff, "1,1") || strstr((char*)simcom_7020.AT_buff, "1,5")|| strstr((char*)simcom_7020.AT_buff, "2,1") || strstr((char*)simcom_7020.AT_buff, "2,5")) return true;
    else continue;
  }
  return false;
} 
bool isInit(int retry) {
  AT_flag res;
  while(retry--) {
    send_ATComand("AT");
    res = _readFeedback(1000, "OK");
    if(res == AT_OK) return true;
    else if(res == AT_ERROR) return false;
  }
  return false;
}
bool isconverhex(int retry) {
  AT_flag res;
  while(retry--) {
    send_ATComand("AT+CREVHEX=0\r\n");
    res = _readFeedback(1000, "OK");
    if(res == AT_OK) return true;
    else if(res == AT_ERROR) return false;
  }
  return false;
}
int mqtt_new(client clientMQTT, int timeout, int buf_size, int retry) {
  AT_flag res;
  char buf[200];
  sprintf(buf, "AT+CMQNEW=\"%s\",\"%d\",%d,%d", clientMQTT.uri, clientMQTT.port, timeout, buf_size);
  while (retry--) {
    send_ATComand(buf);
    res =_readFeedback(timeout, "+CMQNEW:");
    if(res == AT_OK) {
      clientMQTT.mqtt_id = simcom_7020.AT_buff[8] - '0';
      return clientMQTT.mqtt_id;
    }
    else if(res == AT_ERROR) return 0;
  }
  return 0;
}
bool mqtt_start(client clientMQTT, int versionMQTT, int keepalive, int clean_session, int retry) {
  if(!mqtt_new(clientMQTT, 50000, 512, 1)) return false;
  AT_flag res;
  char buf[300];
  sprintf(buf, "AT+CMQCON=%d,%d,\"%s\",%d,%d,0,\"%s\",\"%s\"", clientMQTT.mqtt_id, versionMQTT, clientMQTT.client_id, keepalive, clean_session, clientMQTT.user_name, clientMQTT.password); // will flag = 0
  while (retry--) {
    send_ATComand(buf);
    res =_readFeedback(5000, "OK");
    if(res == AT_OK) return true;
    else if(res == AT_ERROR) return false;
  }
  return false;
}

//1. Disconnect from server
//2. Release the client
//3. Stop MQTT Service
bool mqtt_stop(client clientMQTT, int retry) {
  AT_flag res;
  char buf[200];
  sprintf(buf, "AT+CMQDISCON=%d", clientMQTT.mqtt_id);
  while (retry--) {
    send_ATComand(buf);
    res =_readFeedback(1000, "OK");
    if(res == AT_OK) return true;
    else if(res == AT_ERROR) return false;
  }
  return false;
}

// subscribe one topic to server
bool mqtt_subscribe(client clientMQTT, char *topic, int qos, int retry,  void (*mqttSubcribeCB)(char * data)) {
  AT_flag res;
  char buf[200];
  sprintf(buf, "AT+CMQSUB=%d,\"%s\",%d", clientMQTT.mqtt_id, topic, qos);
  while(retry--) {
    send_ATComand(buf);
    res = _readFeedback(3000, "OK");
    if(res == AT_OK) 
    {
      simcom_7020.mqtt_CB = mqttSubcribeCB;
      return true;
    }
    //else if(res == AT_ERROR) return false;
  }
  return false;
}

bool mqtt_message_publish(client clientMQTT, char *data, char *topic,int qos,  int retry) {
  AT_flag res;
  char buf[512];
  int cnt = 0;
  for (int i = 0; i < strlen(data); i++) {
    if (data[i] == '\\' ) {
      cnt++; 
    }
  }
  sprintf(buf, "AT+CMQPUB=%d,\"%s\",%d,0,1,%d,\"%s\"", clientMQTT.mqtt_id, topic, qos, strlen(data)-cnt, data);
  while(retry--)
  {
    send_ATComand(buf);
    res = _readFeedback(3000, "OK");
    if(res == AT_OK) return true;
    //else if(res == AT_ERROR) return false;
  }
  return false;
}

bool get_signal_strength(char *rssi, char *rsrp, char *rsrq, int retry) {
  AT_flag res;
  char buf[50];
  sprintf(buf, "AT+CENG?");
  while (retry--) {
    send_ATComand(buf);
    res =_readFeedback(1000, "+CENG:");
    if(res == AT_OK) {
      char str[100];
      memcpy(str, simcom_7020.AT_buff, strlen((char*)simcom_7020.AT_buff)+1);
      // get first token
      filter_comma((char *)simcom_7020.AT_buff, 4, 5, (char *)rsrp);
      filter_comma((char *)simcom_7020.AT_buff, 5, 6, (char *)rsrq);
      filter_comma((char *)simcom_7020.AT_buff, 6, 7, (char *)rssi);
      return true;
      }
      else if(res == AT_ERROR) return false;
  }
  return false;
}

bool getCellId(int * mcc, int * mnc, char * lac, char * cid, int retry) {
  /* get cid and lac*/
  AT_flag res;
  char buf[200];
  vTaskDelay(100/portTICK_PERIOD_MS);
  sprintf(buf, "AT+CGREG?");
  while (retry--) {
    send_ATComand(buf);
    res =_readFeedback(1000, "+CGREG:");
    if(res == AT_OK) {
      char str[100];
      memcpy(str, simcom_7020.AT_buff, strlen((char *)simcom_7020.AT_buff)+1);
      // get first token
      char * token = strtok(str, ":");
      // get all token
      int i = 1;
      while( token != NULL ) {
        token = strtok(NULL, ",\"");
        if (i == 3) {
          memcpy(lac, token, strlen(token)+1);
        } else if (i == 4) {
          memcpy(cid, token, strlen(token)+1);
        }
        i++;
      }
      break;
    }
    else if(res == AT_ERROR) return false;
  }
  return false;
}
int filter_comma_t(char *respond_data, int begin, int end, char *output)
{
    memset(output, 0, strlen(output));
    int count_filter = 0;
    int lim = 0;
    int start = 0;
    int finish = 0;
    int i = 0;
    for (i = 0; i < strlen(respond_data); i++)
    {
        if ( respond_data[i] == '\"')
        {
            count_filter ++;
            if (count_filter == begin)          start = i+1;
            if (count_filter == end)            finish = i;
        }

    }
    lim = finish - start;
    for (i = 0; i < lim; i++){
        output[i] = respond_data[start];
        start ++;
    }
    output[i] = 0;
    return 0;
}
