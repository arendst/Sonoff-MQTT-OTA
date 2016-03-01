#ifndef USER_WIFI_H_
#define USER_WIFI_H_

#ifndef WIFI_HOSTNAME
#define WIFI_HOSTNAME  "ESP-%06X-%s"
#endif

#define WIFI_STATUS       0
#define WIFI_SMARTCONFIG  1

typedef void (*WifiCallback)(uint8_t);

void WIFI_Connect(uint8_t* ssid, uint8_t* pass, char* hname, WifiCallback cb);
void WIFI_Check(uint8_t param);

#endif /* USER_WIFI_H_ */
