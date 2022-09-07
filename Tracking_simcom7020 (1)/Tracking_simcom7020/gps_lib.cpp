#include "gps_lib.h"
#include "simcom7020.h"

uint8_t Day[10];
uint8_t Time[10];
uint8_t latitude[50];
uint8_t longitude[50];
uint8_t sog[50];
uint8_t acc[10];
int lat_current;
int lon_current;
static const char * TAG = "";   
void init_GPS(uart_port_t uart_num, int tx_io_num, int rx_io_num, int baud_rate)
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
  static int a = 0;
  xTaskCreate(UART_ISR_GPS, "uart_echo_task1", 4096, NULL, 10, NULL);
}
static void UART_ISR_GPS(void *pvParameters){
  uint8_t data1[512];
  int b = 0;
  while (1) {
    if(GPS_ON)
    {
      int len = uart_read_bytes(NUMERO_PORTA_SERIALE1, data1, (512 - 1), 300 / portTICK_PERIOD_MS);
    // Write data back to the UART
      if(len)
      {
        data1[len] = '\0';
        strcpy((char *)gps_t.GPS_RX_DATA, (char*) data1);
        //ESP_LOGI(TAG, "Rec: %s", (char*) data1);
        GPS_RX_Process();
    }
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}
static void GPS_RX_Process(void)
{
    if (strstr((const char*)gps_t.GPS_RX_DATA, "$GPRMC")&&!Flag_gps_fix) 
    {
      char * buff1;
      buff1 = strstr((const char*)gps_t.GPS_RX_DATA, "$GPRMC");
      ESP_LOGI(TAG, "Rec: %s", (char*) buff1);
      filter_comma((char *)buff1, 1, 2, (char *)Time);
      filter_comma((char *)buff1, 3, 4, (char *)latitude);
      filter_comma((char *)buff1, 5, 6, (char *)longitude);
      filter_comma((char *)buff1, 7, 8, (char *)sog);
      filter_comma((char *)buff1, 9, 10, (char *)Day);
      
      Conver_DateTime((char *)Time, 'T');
      gps_t.latitude = atof((const char *)latitude);
      lat_current = (int )gps_t.latitude/100;
      gps_t.latitude = lat_current + ((gps_t.latitude - lat_current*100)/60);
      
      gps_t.longitude = atof((const char *)longitude);
      lon_current = (int )gps_t.longitude/100;
      gps_t.longitude = lon_current + ((gps_t.longitude - lon_current*100)/60);
      
      gps_t.speed_k = atof((const char *)sog);
      Conver_DateTime((char *)Day, 'D');
      sprintf((char *)Datetime, "%d-%d-%d %d:%d:%d", DateTime.year, DateTime.month, DateTime.day,DateTime.hour,DateTime.minute, DateTime.second );
      if(gps_t.latitude != 0 && gps_t.longitude != 0)
      {
        Flag_gps_fix = true;
      }
    }        
    if (strstr((const char*)gps_t.GPS_RX_DATA, "$GPGGA") && !Flag_gps_acc) 
    {
        char * buff2;
        buff2 = strstr((const char*)gps_t.GPS_RX_DATA, "$GPGGA");
        memset(latitude, 0, strlen((const char *)latitude));
        memset(acc, 0, strlen((const char *)acc));
        filter_comma((char *)buff2, 8, 9, (char *)acc);
        filter_comma((char *)buff2, 2, 3, (char *)latitude);
        gps_t.course_d = atof((const char *)acc);
        if(atof((const char *)latitude) != 0)
        {
          Flag_gps_acc = true;
        }
    }
    if(Flag_gps_acc&&Flag_gps_fix)
    {
      GPS_ON = false;
    }
    memset((char *)gps_t.GPS_RX_DATA, 0, sizeof(gps_t.GPS_RX_DATA));
    gps_t.GPS_RX_INDEX = 0;
}
int filter_comma(char *respond_data, int begin, int end, char *output)
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
static void Conver_DateTime(char *datetime, char kind)
{
  char cv[50];
  if(kind == 'D')
  {
      strncpy(cv, datetime + 0, 2);
      DateTime.day = atoi(cv);
      strncpy(cv, datetime + 2, 2);
      DateTime.month = atoi(cv);
      strncpy(cv, datetime + 4, 2);
      DateTime.year = atoi(cv) + 2000;
  }
  else
  {
      strncpy(cv, datetime + 0, 2);
      DateTime.hour = atoi(cv);
      if(DateTime.hour >= 24)
      {
        DateTime.hour -= 24;
      }
      strncpy(cv, datetime + 2, 2);
      DateTime.minute = atoi(cv);
      strncpy(cv, datetime + 4, 2);
      DateTime.second = atoi(cv);
  }
}
time_t string_to_seconds(const char *timestamp_str)
{
    struct tm tm;
    time_t seconds;
    int r;

    if (timestamp_str == NULL) {
        printf("null argument\n");
        return (time_t)-1;
    }
    r = sscanf(timestamp_str, "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    if (r != 6) {
        printf("expected %d numbers scanned in %s\n", r, timestamp_str);
        return (time_t)-1;
    }

    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    tm.tm_isdst = 0;
    seconds = mktime(&tm);
    if (seconds == (time_t)-1) {
        printf("reading time from %s failed\n", timestamp_str);
    }

    return seconds;
}
