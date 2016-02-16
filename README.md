## Sonoff-MQTT-OTA
Provide ESP8266 based [itead Sonoff](https://www.itead.cc/sonoff-wifi-wireless-switch.html) with MQTT and OTA firmware.
## Prerequisite
Install ESP8266 development environment from [esp8266-dev](https://github.com/nqd/esp8266-dev).

- Create directory ```esp8266-dev/work``` and copy directory ```sonoff``` and file ```espupload.php``` into it.

- Copy file ```Makefile.common``` to directory ```esp8266-dev``` replacing the current file.

Install a local webserver for OTA and copy directory ```api``` in webroot.
## Compile and upload
Compile source with ```make``` and flash once to sonoff using cable connection as shown in [Peter Scargill's blog](http://tech.scargill.net/itead-slampher-and-sonoff) with ```make flash```. Do not connect AC power during the flash cable connection. 

Note: this software is not compatible with Pete's implementation.

Compile OTA images with ```make clean; make IMAGE=1; make register; make clean; make IMAGE=2; make register```.
## Usage
The button on sonoff provides the following features:

- a short press toggles the relay. This will blink the LED twice and sends a MQTT status message like ```stat/sonoff/POWER on```.

- three short presses start Wifi smartconfig which allows for SSID and Password configuration using a mobile phone. The green LED will blink during the smartconfig period for 100 seconds. The MQTT server still needs to be configured using the ```user_config.h``` file.

- four short presses start 'Over the Air' or OTA download of firmware. The green LED is lit during the update. 

- pressing the button for over four seconds resets settings to defaults as defined in ```user_config.h``` and reboots  sonoff.


Sonoff responds to the following MQTT commands:

- The relay can be controlled by ```cmnd/sonoff/power on```, ```cmns/sonoff/power off``` or ```cmnd/sonoff/power toggle```. The LED will blink twice and sends a MQTT status message like ```stat/sonoff/POWER on```. The same function can be initiated with ```cmnd/sonoff/light on```.

- The MQTT topic can be changed with ```cmnd/sonoff/topic sonoff1``` which reboots sonoff and makes it available for MQTT commands like ```cmnd/sonoff1/power on```.

- The OTA firmware location can be made known to sonoff by ```cmnd/sonoff/otaurl http://sidnas2:80/api/sonoff/user1.bin``` where sidnas2 is your webserver hosting the firmware. Reset to default with ```cmnd/sonoff/otaurl 1```.

- Upgrade OTA firmware by ```cmnd/sonoff/upgrade 1```.

- Show status information by ```cmnd/sonoff/status 1```.


Most MQTT commands will result in a status feedback like ```stat/sonoff/POWER On```.


See the code for other features.
