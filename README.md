## Sonoff-MQTT-OTA
Provide ESP8266 based [itead Sonoff](https://www.itead.cc/sonoff-wifi-wireless-switch.html) with MQTT and 'Over the Air' or OTA firmware.
## Prerequisite
Install the ESP8266 development environment from [esp8266-dev](https://github.com/nqd/esp8266-dev).

Copy the sonoff code files as follows:

- Create directory ```esp8266-dev/work``` and copy directory ```sonoff``` and file ```espupload.php``` into it
- Replace file ```Makefile.common``` in directory ```esp8266-dev```

Install php and a local web server (ie apache) for OTA and copy directory ```api``` in webroot.
## Compile and upload
Update ```sonoff/user/user_config.h``` with your specific Wifi and MQTT parameters.

Compile source with ```make``` and flash once to sonoff using cable connection as shown in [Peter Scargill's blog](http://tech.scargill.net/itead-slampher-and-sonoff) with ```make flash```. **Do not connect AC power during the flash cable connection**. 

Compile and upload OTA images to your web server with ```make clean; make IMAGE=1; make register; make clean; make IMAGE=2; make register```.

Note: this software is not compatible with Pete's implementation.
## Usage
The button on sonoff provides the following features:

- a short press toggles the relay. This will blink the LED twice and sends a MQTT status message like ```stat/sonoff/POWER on```
- three short presses start Wifi smartconfig which allows for SSID and Password configuration using an Android mobile phone with the [ESP8266 SmartConfig](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch) app. The green LED will blink during the smartconfig period for 100 seconds. The MQTT server still needs to be configured using the ```user_config.h``` file
- four short presses start OTA download of firmware. The green LED is lit during the update
- pressing the button for over four seconds resets settings to defaults as defined in ```user_config.h``` and reboots  sonoff

Sonoff responds to the following MQTT commands:

- the relay can be controlled by ```cmnd/sonoff/power on```, ```cmns/sonoff/power off``` or ```cmnd/sonoff/power toggle```. The LED will blink twice and sends a MQTT status message like ```stat/sonoff/POWER on```. The same function can be initiated with ```cmnd/sonoff/light on```
- the MQTT topic can be changed with ```cmnd/sonoff/topic sonoff1``` which reboots sonoff and makes it available for MQTT commands like ```cmnd/sonoff1/power on```
- the OTA firmware location can be made known to sonoff by ```cmnd/sonoff/otaurl http://sidnas2:80/api/sonoff/user1.bin``` where sidnas2 is your webserver hosting the firmware. Reset to default with ```cmnd/sonoff/otaurl 1```
- upgrade OTA firmware by ```cmnd/sonoff/upgrade 1```
- show status information by ```cmnd/sonoff/status 1```

Most MQTT commands will result in a status feedback like ```stat/sonoff/POWER On```.
## Commands supported
The firmware supports both a **serial** and a **MQTT** Man Machine interface. The serial interface is set to 115200 bps. The MQTT commands are constructed as ```cmnd/sonoff/<command>```. 

The following commands are recognised by both topic and grouptopic:

Command | Description
------- | -----------
power | Show current power state as On or Off
power on | Turn power On
power off | Turn power Off
power toggle | Toggle power
power 1 | Turn power On
power 0 | Turn power Off
power 2 | Toggle power
light | Show current power state as On or Off
light on | Turn power On
light off | Turn power Off
light toggle | Toggle power
light 1 | Turn power On
light 0 | Turn power Off
light 2 | Toggle power
status | Show abbreviated status information
status 1 | Show all status information
status 2 | Show version information
grouptopic | Show current MQTT group topic
grouptopic 1 | Reset MQTT group topic to ```user_config.h``` value and restart
grouptopic your-grouptopic | Set MQTT group topic and restart
timezone | Show current timezone
timezone -12 .. 12 | Set timezone

The following commands are recognised by topic only:

Command | Description
------- | -----------
restart 1 | Restart sonoff
reset 1 | Reset sonoff parameters to ```user_config.h``` values and restart
ssid | Show current Wifi SSId
ssid 1 | Reset Wifi SSId to ```user_config.h``` value and restart
ssid your-ssid | Set Wifi SSId and restart
password | Show current Wifi password
password 1 | Reset Wifi password to ```user_config.h``` value and restart
password your-password | Set Wifi password and restart
host | Show current MQTT host
host 1 | Reset MQTT host to ```user_config.h``` value and restart
host your-host | Set MQTT host and restart
topic | Show current MQTT topic
topic 1 | Reset MQTT topic to ```user_config.h``` value and restart
topic your-topic | Set MQTT topic and restart
smartconfig 1 | Start smart config
otaurl | Show current otaurl
otaurl 1 | Reset otaurl to ```user_config.h``` value
otaurl your-otaurl | Set otaurl
upgrade 1 | Download ota firmware from your web server and restart

If the same topic has been defined to more than one sonoff an individual sonoff can still be addressed by the fall back topic MQTT_CLIENT_ID as defined in user_config.h. The fall back topic will be ```DVES_<last six characters of MAC address>```.
## Tips
- To aid in finding the IP address of sonoff the network name will be ```ESP-<last six characters of MAC address>-<MQTT topic>```. So the default name is ```ESP-123456-sonoff```
- The initial firmware from ```api/sonoff/user1.bin``` can be flashed using the SDK 1.4 provided bin files with the following esptool.py command line:
```esptool.py --port /dev/ttyUSB0 write_flash -fs 8m 0x00000 boot_v1.4\(b1\).bin 0x01000 user1.bin 0xFC000 esp_init_data_default.bin 0xFE000 blank.bin```
- Use the group topic to address several sonoffs with one (restricted) MQTT command
