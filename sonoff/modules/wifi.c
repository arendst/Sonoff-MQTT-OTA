/* wifi.c
*
* Copyright (c) 2016, Theo Arends
*
*/
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "smartconfig.h"
#include "wifi.h"
#include "debug.h"

#define CHKSECS  20   // seconds

static os_timer_t wifi_reboot_timer;
WifiCallback wifiCb = NULL;
uint8_t wificounter = CHKSECS;
uint8_t *newssid, *newpass;
static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;

void wifi_reboot_cb(void *arg)
{
  INFO("WIFI Smartconfig: Will reboot now\n");
  system_restart();
}

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
  switch (status) {
  case SC_STATUS_WAIT:
    INFO("WIFI Smartconfig: SC_STATUS_WAIT\n");
    break;
  case SC_STATUS_FIND_CHANNEL:
    INFO("WIFI Smartconfig: SC_STATUS_FIND_CHANNEL\n");
    break;
  case SC_STATUS_GETTING_SSID_PSWD:
    INFO("WIFI Smartconfig: SC_STATUS_GETTING_SSID_PSWD\n");
    sc_type *type = pdata;
    if (*type == SC_TYPE_ESPTOUCH) {
      INFO("WIFI Smartconfig: SC_TYPE:SC_TYPE_ESPTOUCH\n");
    } else {
      INFO("WIFI Smartconfig: SC_TYPE:SC_TYPE_AIRKISS\n");
    }
    break;
  case SC_STATUS_LINK:
    INFO("WIFI Smartconfig: SC_STATUS_LINK\n");
    struct station_config *sta_conf = pdata;
    INFO("WIFI Smartconfig: ssid %s\n", sta_conf->ssid);
    INFO("WIFI Smartconfig: password %s\n", sta_conf->password);
    os_memcpy(newssid, sta_conf->ssid, strlen(sta_conf->ssid)+1);
    os_memcpy(newpass, sta_conf->password, strlen(sta_conf->password)+1);
    wifi_station_set_config(sta_conf);
    wifi_station_disconnect();
    wifi_station_connect();
    break;
  case SC_STATUS_LINK_OVER:
    INFO("WIFI Smartconfig: SC_STATUS_LINK_OVER\n");
    if (pdata != NULL) {
      uint8 phone_ip[4] = {0};

      os_memcpy(phone_ip, (uint8*)pdata, 4);
      INFO("WIFI Smartconfig: Phone ip %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
    }
    smartconfig_stop();
    // 5 seconds delay before reboot to allow for save settings in flash
    os_timer_disarm(&wifi_reboot_timer);
    os_timer_setfn(&wifi_reboot_timer, (os_timer_func_t *)wifi_reboot_cb, (void *)0);
    os_timer_arm(&wifi_reboot_timer, 5000, 0);
    break;
  }
}

void ICACHE_FLASH_ATTR
wifi_smartconfig()
{
  wificounter = 102;  // Allow up to 100 seconds for phone to provide ssid/pswd
  wifi_station_set_auto_connect(FALSE);
  wifi_station_disconnect();
  wifi_set_opmode(STATION_MODE);
  smartconfig_set_type(SC_TYPE_ESPTOUCH);  // SC_TYPE_ESPTOUCH, SC_TYPE_AIRKISS, SC_TYPE_ESPTOUCH_AIRKISS
  smartconfig_start(smartconfig_done);
  os_timer_disarm(&wifi_reboot_timer);
  os_timer_setfn(&wifi_reboot_timer, (os_timer_func_t *)wifi_reboot_cb, (void *)0);
  os_timer_arm(&wifi_reboot_timer, (wificounter -2) *1000, 0);
}

static void ICACHE_FLASH_ATTR
wifi_check_ip()
{
  struct ip_info ipConfig;

  wifi_get_ip_info(STATION_IF, &ipConfig);
  wifiStatus = wifi_station_get_connect_status();
  if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0) {
    wificounter = CHKSECS;
  } else {
    switch (wifi_station_get_connect_status()) {
      case STATION_WRONG_PASSWORD:
      case STATION_NO_AP_FOUND:
      case STATION_CONNECT_FAIL:
        INFO("WIFI: STATION_CONNECT_FAIL\r\n");
        wifi_smartconfig();
        break;
      default:
        INFO("WIFI: STATION_IDLE\r\n");
        wificounter = 1;
        break;
    }
  }
  if (wifiStatus != lastWifiStatus) {
    lastWifiStatus = wifiStatus;
    if(wifiCb) wifiCb(wifiStatus);
  }
}

void ICACHE_FLASH_ATTR
WIFI_Check(uint8_t param)
{
  wificounter--;
  switch (param) {
    case WIFI_SMARTCONFIG:
      INFO("WIFI: WIFI_SMARTCONFIG\r\n");
      wifi_smartconfig();
      break;
    default:
      if (wificounter <= 0) {
        INFO("WIFI: WIFI_STATUS\r\n");
        wificounter = CHKSECS;
        wifi_check_ip();
      }
      break;
  }
}

void ICACHE_FLASH_ATTR
WIFI_Connect(uint8_t* ssid, uint8_t* pass, char* hname, WifiCallback cb)
{
  struct station_config stationConf;
  char hostname[32];

  INFO("WIFI: WIFI_INIT\r\n");
  newssid = ssid;
  newpass = pass;
  os_sprintf(hostname, WIFI_HOSTNAME, system_get_chip_id(), hname);
  wifiCb = cb;

  wifi_set_opmode(STATION_MODE);
  wifi_station_set_auto_connect(FALSE);

  os_memset(&stationConf, 0, sizeof(struct station_config));

  os_sprintf(stationConf.ssid, "%s", ssid);
  os_sprintf(stationConf.password, "%s", pass);

  wifi_station_set_config(&stationConf);

  if(wifi_get_phy_mode() != PHY_MODE_11N)
    wifi_set_phy_mode(PHY_MODE_11N);

  wificounter = 2;

  wifi_station_set_auto_connect(TRUE);
  wifi_station_set_hostname(hostname);
  wifi_station_connect();
}
