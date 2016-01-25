# Sonoff-MQTT-OTA
Provide iThead Sonoff with MQTT and OTA firmware.
# Prerequisite
Install ESP8266 development environment from [esp8266-dev](https://github.com/nqd/esp8266-dev).
Install a local webserver and copy api folder in webroot.
# Compile
Compile source and flash once to sonoff using cable connection as shown in [Peter Scargill's blog](http://tech.scargill.net/itead-slampher-and-sonoff).
Note: this software is not compatible with his implementation.
Future updates can be uploaded by OTA using the MQTT command ```cmnd/sonoff/upgrade 1``` or pressing the button quickly four times.
