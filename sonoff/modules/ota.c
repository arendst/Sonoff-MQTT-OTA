/*
    ota for the ESP8266
    Copyright (C) 2015 Theo Arends
    Based on work by https://github.com/nqd/esp8266-dev
*/

#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "upgrade.h"
#include "ota.h"
#include "debug.h"

#define pHeadStatic "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: ESP8266\r\n\
Accept: */*\r\n\
Content-Type: application/json\r\n"

typedef struct {
  char *host;
  char *url;
  uint16_t port;
  struct espconn *conn;
  struct upgrade_server_info *up_server;
  void *reverse;
} ota_t;

static os_timer_t ota_timer;
static os_timer_t ota_restart_timer;
static ota_t ota_cdn;
static ota_restart_flg;

LOCAL void ICACHE_FLASH_ATTR
ota_restart()
{
  if (ota_restart_flg == 0) {
    ota_restart_flg = 1;
    os_timer_disarm(&ota_restart_timer);
    os_timer_setfn(&ota_restart_timer, (os_timer_func_t *)ota_restart, NULL);
    os_timer_arm(&ota_restart_timer, 2000, 0);
  } else {
    system_restart();
  }
}

LOCAL void ICACHE_FLASH_ATTR
ota_start_esp_connect(struct espconn *conn, void *connect_cb, void *disconnect_cb, void *reconn_cb)
{
  espconn_regist_connectcb(conn, connect_cb);
  espconn_regist_disconcb(conn, disconnect_cb);
  espconn_regist_reconcb(conn, reconn_cb);

  if (espconn_connect(conn) != 0) {
    INFO("OTA: Connect fail\n");
    ota_restart();
  }
}

LOCAL void ICACHE_FLASH_ATTR
ota_response(void *arg)
{
  struct upgrade_server_info *server = arg;

  if(server->upgrade_flag == true) {
    INFO("OTA: Firmware upgrade success\n");
    system_upgrade_reboot();
  }
  else {
    INFO("OTA: Firmware upgrade fail\n");
    ota_restart();
  }
}

LOCAL void ICACHE_FLASH_ATTR
ota_recon_cb(void *arg, sint8 err)
{
  INFO("OTA: Firmware upgrade reconnect callback, error code %d\n", err);
  ota_restart();
}

LOCAL void ICACHE_FLASH_ATTR
ota_discon_cb(void *arg)
{
  INFO("OTA: Firmware client disconnect\n");
  ota_restart();
}

LOCAL void ICACHE_FLASH_ATTR
ota_connect_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  ota_t *ota_client = (ota_t *)pespconn->reverse;
  char temp[32] = {0};
  uint8_t i = 0;

  INFO("OTA: Firmware client connected\n");
  system_upgrade_init();

  ota_client->up_server = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
  ota_client->up_server->upgrade_version[5] = '\0';       // no, we dont use this
  ota_client->up_server->pespconn = pespconn;
  os_memcpy(ota_client->up_server->ip, pespconn->proto.tcp->remote_ip, 4);
  ota_client->up_server->port = ota_client->port;
  ota_client->up_server->check_cb = ota_response;
//  ota_client->up_server->check_times = 60000;
  ota_client->up_server->check_times = 20000;
  if(ota_client->up_server->url == NULL) {
    ota_client->up_server->url = (uint8 *) os_zalloc(1024);
  }

  os_sprintf(ota_client->up_server->url, "GET %s HTTP/1.1\r\nHost: %s\r\n"pHeadStatic"\r\n", ota_client->url, ota_client->host);
  if(system_upgrade_start(ota_client->up_server) == false) {
    INFO("OTA: Fail to start system upgrade\n");
    ota_restart();
  }
}

LOCAL void ICACHE_FLASH_ATTR
ota_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  ota_t *ota_client = (ota_t *)pespconn->reverse;

  if(ipaddr == NULL) {
    INFO("OTA: DNS Found, but got no ip\n");
    ota_restart();
  }

  INFO("OTA: DNS found ip %d.%d.%d.%d\n",
      *((uint8 *) &ipaddr->addr),
      *((uint8 *) &ipaddr->addr + 1),
      *((uint8 *) &ipaddr->addr + 2),
      *((uint8 *) &ipaddr->addr + 3));

  if(ipaddr->addr != 0) {
    os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
  }
  ota_start_esp_connect(ota_client->conn, ota_connect_cb, ota_discon_cb, ota_recon_cb);
}

LOCAL void ICACHE_FLASH_ATTR
start_ota_cb(ota_t *ota_client)
{
  // connection
  ota_client->conn = (struct espconn *)os_zalloc(sizeof(struct espconn));
  ota_client->conn->reverse = (void*)ota_client;
  ota_client->conn->type = ESPCONN_TCP;
  ota_client->conn->state = ESPCONN_NONE;
  // new tcp connection
  ota_client->conn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
  ota_client->conn->proto.tcp->local_port = espconn_port();
  ota_client->conn->proto.tcp->remote_port = ota_client->port;

  // if ip address is provided, go ahead
  if (UTILS_StrToIP(ota_client->host, &ota_client->conn->proto.tcp->remote_ip)) {
    INFO("OTA: Firmware client will connect to %s:%d\r\n", ota_client->host, ota_client->port);
    ota_start_esp_connect(ota_client->conn, ota_connect_cb, ota_discon_cb, ota_recon_cb);
  }
  // else, use dns query to get ip address
  else {
    INFO("OTA: Firmware client will connect to domain %s:%d\r\n", ota_client->host, ota_client->port);
    espconn_gethostbyname(ota_client->conn, ota_client->host, (ip_addr_t *)(ota_client->conn->proto.tcp->remote_ip), ota_dns_found);
  }
}

/*
* Only http is supported but a different port may be used.
* The following urls are handled correctly:
*   http://sidnas2:80/api/sensor1/user1.bin
*   http://sidnas2/api/sensor1/user1.bin
*   sidnas2/api/sensor1/user1.bin
*/
void ICACHE_FLASH_ATTR
start_ota(char *url)
{
  int i;
  char *str, *str2, *p, *q;
  ota_t *ota_client = &ota_cdn;

  ota_restart_flg = 0;

  os_memset(ota_client, '\0', sizeof(ota_t));
  str = strtok_r(url, "/", &p);     // http: or sidnas2:80 or sidnas2
  str2 = strstr(str, ":");          // : or :80 or null
  if (str2 && os_strlen(str2) == 1) {
    p++;                            // skip http://
    str = strtok_r(NULL, "/", &p);  // sidnas2:80 or sidnas2
    str2 = strstr(str, ":");        // :80 or null
  }
  if (str2) {
    i = str2 - str;
    str[i] = 0;                     // sidnas2
    ota_client->host = (char*)os_zalloc(os_strlen(str)+1);
    os_strncpy(ota_client->host, str, os_strlen(str));
    str[i] = ':';                   // sidnas2:80
    str2++;
    ota_client->port = atoi(str2);
  } else {
    ota_client->host = (char*)os_zalloc(os_strlen(str)+1);
    os_strncpy(ota_client->host, str, os_strlen(str));
    ota_client->port = 80;
  }
  i = os_strlen(str);
  str[i] = '/';
  p--;
  str = strtok_r(NULL, ".", &p);    // /api/sensor1/user1
  i = os_strlen(str);
  str[i] = '.';                     // /api/sensor1/user1.bin
  if(system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
    str[i-1] = '2';                 // /api/sensor1/user2.bin
  if(system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
    str[i-1] = '1';                 // /api/sensor1/user1.bin
  ota_client->url = (char*)os_zalloc(os_strlen(str)+1);
  os_strncpy(ota_client->url, str, os_strlen(str));

  os_timer_disarm(&ota_timer);
  os_timer_setfn(&ota_timer, (os_timer_func_t *)start_ota_cb, ota_client);
  os_timer_arm(&ota_timer, 2000, 0);
}
