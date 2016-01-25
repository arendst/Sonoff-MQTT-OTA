## Sonoff-MQTT-OTA
Provide iThead Sonoff with MQTT and OTA firmware.
## Prerequisite
Install ESP8266 development environment from [esp8266-dev](https://github.com/nqd/esp8266-dev).
Install a local webserver and copy api folder in webroot.
## Compile and upload
Compile source and flash once to sonoff using cable connection as shown in [Peter Scargill's blog](http://tech.scargill.net/itead-slampher-and-sonoff).

Note: this software is not compatible with his implementation.

Future updates can be uploaded by OTA using the MQTT command ```cmnd/sonoff/upgrade 1``` or pressing the button quickly four times.
## Usage
Using MQTT the relay can be controlled with ```cmnd/sonoff/power on``` or ```cmnd/sonoff/power off```.

The code also supports Wifi smartconfig but the MQTT server still needs to be made known using the ```user_config.h``` file.

The MQTT topic can be changed with ```cmnd/sonoff/topic sonoff1``` which reboots the sonoff and makes it available for MQTT commands like ```cmnd/sonoff1/power on```.

Most MQTT commands will result in a status feedback like ```stat/sonoff/POWER On```. Switching the power using the button also provide a MQTT status feedback.

See the code for other features.
