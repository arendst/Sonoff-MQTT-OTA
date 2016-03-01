#ifndef _MQTT_CONFIG_H_
#define _MQTT_CONFIG_H_

#include "user_config.h"

#ifndef MQTT_BUF_SIZE
#define MQTT_BUF_SIZE         1024
#endif

#ifndef MQTT_RECONNECT_TIMEOUT
#define MQTT_RECONNECT_TIMEOUT      5 /*second*/
#endif

#ifndef QUEUE_BUFFER_SIZE
#define QUEUE_BUFFER_SIZE     2048
#endif

#ifndef MQTT_SSL_ENABLE
#define MQTT_SSL_ENABLE
#endif

#ifndef MQTT_SSL_SIZE
#define MQTT_SSL_SIZE         4096
#endif

#define PROTOCOL_NAMEv31  /*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311     /*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif