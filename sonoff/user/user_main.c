/* main.c -- ota MQTT sonoff
*
* Copyright (c) 2016, Theo Arends
*
*/
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "config.h"
#include "user_interface.h"
#include "mem.h"
#include "wifi.h"
#include "mqtt.h"
#include "ota.h"
#include "rtc.h"
#include "gpio.h"

#if TDEBUG
#define INFO(...) os_printf(__VA_ARGS__)
#else
#define INFO(...)
#endif

#define REL_PIN       12             // GPIO 12 = Red Led and Relay (0 = Off, 1 = On)
#define REL_MUX       PERIPHS_IO_MUX_MTDI_U
#define REL_FUNC      FUNC_GPIO12

#define LED_PIN       13             // GPIO 13 = Green Led (0 = On, 1 = Off)
#define LED_MUX       PERIPHS_IO_MUX_MTCK_U
#define LED_FUNC      FUNC_GPIO13

#define KEY_PIN       0              // GPIO 00 = Button
#define KEY_MUX       PERIPHS_IO_MUX_GPIO0_U
#define KEY_FUNC      FUNC_GPIO0
#define PRESSED       0
#define NOT_PRESSED   1

#define STATES        10             // loops per second

static os_timer_t state_timer;
uint8_t state = 0;
uint8_t otaflag = 0;
uint8_t restartflag = 0;
uint8_t smartconfigflag = 0;
uint16_t heartbeat = 0;

MQTT_Client mqttClient;

uint8_t lastbutton = NOT_PRESSED;
uint8_t holdcount = 0;
uint8_t multiwindow = 0;
uint8_t multipress = 0;
uint16_t blinks = 3;
uint8_t blinkstate = 1;

// we do not use this app as SSL server
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

void ICACHE_FLASH_ATTR
wifiConnectCb(uint8_t status)
{
  if(status == STATION_GOT_IP){
    MQTT_Connect(&mqttClient);
  } else {
    MQTT_Disconnect(&mqttClient);
  }
}

void ICACHE_FLASH_ATTR
MQTT_Publish_Tx(MQTT_Client *client, const char* topic, const char* data, int data_length, int qos, int retain)
{
  MQTT_Publish(client, topic, data, data_length, qos, retain);
#ifdef SERIAL_IO
  os_printf("%s = %s\n", strrchr(topic,'/')+1, data);
#endif
}

void ICACHE_FLASH_ATTR
mqttConnectedCb(uint32_t *args)
{
  char stopic[40], svalue[40];

  MQTT_Client* client = (MQTT_Client*)args;
  INFO("MQTT: Connected\r\n");

  os_sprintf(stopic, "%s/%s/#", SUB_PREFIX, sysCfg.mqtt_topic);
  MQTT_Subscribe(client, stopic, 0);
  os_sprintf(stopic, "%s/"MQTT_CLIENT_ID"/#", SUB_PREFIX, system_get_chip_id());  // Fall back topic
  MQTT_Subscribe(client, stopic, 0);

  os_sprintf(stopic, "%s/%s/NAME", PUB_PREFIX, sysCfg.mqtt_topic);
  os_sprintf(svalue, "Sonoff switch");
  MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
  os_sprintf(stopic, "%s/%s/VERSION", PUB_PREFIX, sysCfg.mqtt_topic);
  os_sprintf(svalue, "%s", VERSION);
  MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
  os_sprintf(stopic, "%s/%s/FALLBACKTOPIC", PUB_PREFIX, sysCfg.mqtt_topic);
  os_sprintf(svalue, MQTT_CLIENT_ID, system_get_chip_id());
  MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
}

void ICACHE_FLASH_ATTR
mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
  uint8_t i, grpflg = 0;
  char *str, *p, *mtopic = NULL, *type = NULL;
  char stopic[40], svalue[200];

  char *topicBuf = (char*)os_zalloc(topic_len+1),
       *dataBuf = (char*)os_zalloc(data_len+1),
       *dataBufUc = (char*)os_zalloc(data_len+1);

  MQTT_Client* client = (MQTT_Client*)args;

  os_memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;
  i = 0;
  for (str = strtok_r(topicBuf, "/", &p); str && i < 3; str = strtok_r(NULL, "/", &p))
  {
    switch (i) {
    case 0:  // CMND
      break;
    case 1:  // TOPIC / DVES_123456
      mtopic = str;
      break;
    case 2:  // TEXT
      type = str;
    }
    i++;
  }
  if (type != NULL) for(i = 0; i < strlen(type); i++) type[i] = toupper(type[i]);

  os_memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;
  for(i = 0; i <= data_len; i++) dataBufUc[i] = toupper(dataBuf[i]);

  INFO("MQTT DataCb: Topic = %s, Type = %s, data = %s (%s) \r\n", mtopic, type, dataBuf, dataBufUc);

  if (type != NULL) {
    blinks = 2;
    os_sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, type);
    strcpy(svalue, "Error");

    uint16_t payload = atoi(dataBuf);
    if (!strcmp(dataBufUc,"OFF")) payload = 0;
    if (!strcmp(dataBufUc,"ON")) payload = 1;
    if (!strcmp(dataBufUc,"TOGGLE")) payload = 2;

    if (!strcmp(type,"STATUS")) {
      os_sprintf(svalue, "%s, %s, %s, %d, %d",
        VERSION, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic, sysCfg.power, sysCfg.timezone);
      if ((data_len > 0) && (payload == 1)) {
        os_sprintf(svalue, "%s, "MQTT_CLIENT_ID", %s, %s, %s, %s, %d",
          svalue, system_get_chip_id(), sysCfg.otaUrl, sysCfg.sta_ssid, sysCfg.sta_pwd, sysCfg.mqtt_host, heartbeat);
      }
    }
    else if (!strcmp(type,"UPGRADE")) {
      if ((data_len > 0) && (payload == 1)) {
        blinks = 4;
        otaflag = 3;
        os_sprintf(svalue, "Upgrade %s", VERSION);
      }
      else
        os_sprintf(svalue, "1 to upgrade");
    }
    else if (!strcmp(type,"OTAURL")) {
      if ((data_len > 0) && (data_len < 80))
        strcpy(sysCfg.otaUrl, ((payload == 1) ? OTA_URL : dataBuf));
      os_sprintf(svalue, "%s", sysCfg.otaUrl);
    }
    else if (!strcmp(type,"SSID")) {
      if ((data_len > 0) && (data_len < 32)) {
        strcpy(sysCfg.sta_ssid, ((payload == 1) ? STA_SSID : dataBuf));
        restartflag = 2;
      }
      os_sprintf(svalue, "%s", sysCfg.sta_ssid);
    }
    else if (!strcmp(type,"PASSWORD")) {
      if ((data_len > 0) && (data_len < 64)) {
        strcpy(sysCfg.sta_pwd, ((payload == 1) ? STA_PASS : dataBuf));
        restartflag = 2;
      }
      os_sprintf(svalue, "%s", sysCfg.sta_pwd);
    }
    else if (!strcmp(type,"HOST")) {
      if ((data_len > 0) && (data_len < 32)) {
        strcpy(sysCfg.mqtt_host, ((payload == 1) ? MQTT_HOST : dataBuf));
        restartflag = 2;
      }
      os_sprintf(svalue, "%s", sysCfg.mqtt_host);
    }
    else if (!strcmp(type,"TOPIC")) {
      if ((data_len > 0) && (data_len < 32)) {
        for(i = 0; i <= data_len; i++)
          if ((dataBuf[i] == '/') || (dataBuf[i] == '+') || (dataBuf[i] == '#')) dataBuf[i] = '_';
        os_sprintf(svalue, MQTT_CLIENT_ID, system_get_chip_id());
        if (!strcmp(dataBuf, svalue)) payload = 1;
        strcpy(sysCfg.mqtt_topic, (payload == 1) ? MQTT_TOPIC : dataBuf);
        restartflag = 2;
      }
      os_sprintf(svalue, "%s", sysCfg.mqtt_topic);
    }
    else if (!strcmp(type,"RESTART")) {
      if ((data_len > 0) && (payload == 1)) {
        restartflag = 2;
        os_sprintf(svalue, "Restarting");
      } else
        os_sprintf(svalue, "1 to restart");
    }
    else if (!strcmp(type,"RESET")) {
      if ((data_len > 0) && (payload == 1)) {
        CFG_Default();
        restartflag = 2;
        os_sprintf(svalue, "Reset and Restarting");
      } else
        os_sprintf(svalue, "1 to reset");
    }
    else if (!strcmp(type,"TIMEZONE")) {
      if ((data_len > 0) && (payload >= -12) && (payload <= 12)) {
        sysCfg.timezone = payload;
        rtc_timezone(sysCfg.timezone);
      }
      os_sprintf(svalue, "%d", sysCfg.timezone);
    }
    else if ((!strcmp(type,"LIGHT")) || (!strcmp(type,"POWER"))) {
      os_sprintf(sysCfg.mqtt_subtopic, "%s", type);
      if ((data_len > 0) && (payload >= 0) && (payload <= 2)) {
        switch (payload) {
        case 0: // Off
        case 1: // On
          sysCfg.power = payload;
          break;
        case 2: // Toggle
          sysCfg.power ^= 1;
          break;
        }
        GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
      }
      strcpy(svalue, (sysCfg.power == 0) ? "Off" : "On");
    }
    else {
      type = NULL;
    }
    if (type == NULL) {
      blinks = 1;
      INFO("*** Syntax error \r\n");
      os_sprintf(stopic, "%s/%s/SYNTAX", PUB_PREFIX, sysCfg.mqtt_topic);
      strcpy(svalue, "Status, Upgrade, Otaurl, Restart, Reset, SSId, Password, Host, Topic, Timezone, Light, Power");
    }
    MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
  }
  os_free(topicBuf);
  os_free(dataBuf);
  os_free(dataBufUc);
}

void ICACHE_FLASH_ATTR
send_power()
{
  char stopic[40], svalue[20];

  os_sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic);
  strcpy(svalue, (sysCfg.power == 0) ? "Off" : "On");
  MQTT_Publish_Tx(&mqttClient, stopic, svalue, strlen(svalue), 0, 0);
}

void ICACHE_FLASH_ATTR
send_heartbeat()
{
  char stopic[40], svalue[20];

  heartbeat++;
  os_sprintf(stopic, "%s/%s/HEARTBEAT", PUB_PREFIX, sysCfg.mqtt_topic);
  os_sprintf(svalue, "%d", heartbeat);
  MQTT_Publish_Tx(&mqttClient, stopic, svalue, strlen(svalue), 0, 0);
}

void state_cb(void *arg)
{
  uint8_t button;
  char stopic[128], svalue[128];
  char *token;

  state++;
  if (state == STATES) {             // Every second
    state = 0;
    rtc_second();
    if ((rtcTime.Minute == 0) && (rtcTime.Second == 30)) send_heartbeat();
  }

  if (serialInBuf[0]) {
    token = strtok(serialInBuf, " ");
    os_sprintf(stopic, "%s/%s/%s", SUB_PREFIX, sysCfg.mqtt_topic, token);
    token = strtok(NULL, "");
    os_sprintf(svalue, "%s", (token == NULL) ? "" : token);
    serialInBuf[0] = 0;
    mqttDataCb((uint32_t*)&mqttClient, stopic, strlen(stopic), svalue, strlen(svalue));
  }

  button = GPIO_INPUT_GET(KEY_PIN);
  if ((button == PRESSED) && (lastbutton == NOT_PRESSED)) {
    if (multiwindow == 0) {
      multipress = 1;
      sysCfg.power ^= 1;
      GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
      send_power();
    } else {
      multipress++;
      INFO("Multipress %d\n", multipress);
    }
    blinks = 1;
    multiwindow = STATES;            // 1 second multi press window
  }
  lastbutton = button;
  if (button == NOT_PRESSED) {
    holdcount = 0;
  } else {
    holdcount++;
    if (holdcount == (STATES *4)) {  // 4 seconds button hold
      CFG_Default();
      multipress = 0;
      restartflag = 4;               // Allow 4 second delay to release button
      blinks = 2;
    }
  }
  if (multiwindow) {
    multiwindow--;
  } else {
    switch (multipress) {
    case 3:
      smartconfigflag = 1;
      blinks = 1000;
      break;
    case 4:
      otaflag = 1;
      GPIO_OUTPUT_SET(LED_PIN, 0);
      break;
    }
    multipress = 0;
  }

  if ((blinks) && (state & 1)) {
    blinkstate ^= 1;
    GPIO_OUTPUT_SET(LED_PIN, blinkstate);
    if (blinkstate) blinks--;
  }

  switch (state) {
  case (STATES/10)*2:
    if (otaflag) {
      otaflag--;
      if (otaflag <= 0) {
        os_timer_disarm(&state_timer);
        start_ota(sysCfg.otaUrl);
      }
    }
    break;
  case (STATES/10)*4:
    CFG_Save();
    if (restartflag) {
      restartflag--;
      if (restartflag <= 0) system_restart();
    }
    break;
  case (STATES/10)*6:
    if (smartconfigflag) {
      smartconfigflag = 0;
      WIFI_Check(WIFI_SMARTCONFIG);
    } else {
      WIFI_Check(WIFI_STATUS);
    }
    break;
  }
}

void ICACHE_FLASH_ATTR
user_init(void)
{
  char stopic[32];

  uart_init(BIT_RATE_115200, BIT_RATE_115200);
  os_delay_us(1000000);

  CFG_Load();

  os_printf("\n");
  uart0_tx_buffer("\n", os_strlen("\n"));
  os_printf("Project %s (Topic %s, Fallback "MQTT_CLIENT_ID") Version %s\n",
    PROJECT, sysCfg.mqtt_topic, system_get_chip_id(), VERSION);

  MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, MQTT_PORT, DEFAULT_SECURITY);
  os_sprintf(stopic, MQTT_CLIENT_ID, system_get_chip_id());
  MQTT_InitClient(&mqttClient, stopic, MQTT_USER, MQTT_PASS, MQTT_KEEPALIVE, 1);
  os_sprintf(stopic, "%s/%s/lwt", PUB_PREFIX, sysCfg.mqtt_topic);
  MQTT_InitLWT(&mqttClient, stopic, "offline", 0, 0);
  MQTT_OnConnected(&mqttClient, mqttConnectedCb);
  MQTT_OnData(&mqttClient, mqttDataCb);

  WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, sysCfg.mqtt_topic, wifiConnectCb);

  PIN_FUNC_SELECT(REL_MUX, REL_FUNC);
  GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);

  PIN_FUNC_SELECT(LED_MUX, LED_FUNC);
  GPIO_OUTPUT_SET(LED_PIN, blinkstate);

  PIN_FUNC_SELECT(KEY_MUX, KEY_FUNC);
  PIN_PULLUP_EN(KEY_MUX);            // Enable pull-up
  GPIO_DIS_OUTPUT(KEY_PIN);          // Set KEY_PIN pin as an input

  rtc_init(sysCfg.timezone);

  os_timer_disarm(&state_timer);
  os_timer_setfn(&state_timer, (os_timer_func_t *)state_cb, (void *)0);
  os_timer_arm(&state_timer, 1000 / STATES, 1);

  INFO("System started ...\r\n");
}
