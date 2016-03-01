#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_

typedef struct{
  uint32_t cfg_holder;
  char sta_ssid[32];
  char sta_pwd[64];
  char mqtt_host[32];
  char mqtt_grptopic[32];
  char mqtt_topic[32];
  char otaUrl[80];

  char mqtt_subtopic[32];
  uint8_t timezone;
  uint8_t power;

} SYSCFG;

typedef struct {
    uint8 flag;
    uint8 pad[3];
} SAVE_FLAG;

void ICACHE_FLASH_ATTR CFG_Save();
void ICACHE_FLASH_ATTR CFG_Load();
void ICACHE_FLASH_ATTR CFG_Default();

extern SYSCFG sysCfg;

#endif /* USER_CONFIG_H_ */
