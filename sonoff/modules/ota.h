#ifndef _OTA_H_
#define _OTA_H_

/*
* Only http is supported but a different port may be used.
* The following urls are handled correctly:
*   http://sidnas2:80/api/sensor1/user1.bin
*   http://sidnas2/api/sensor1/user1.bin
*   sidnas2/api/sensor1/user1.bin
*/
void ICACHE_FLASH_ATTR start_ota(char *url);

#endif