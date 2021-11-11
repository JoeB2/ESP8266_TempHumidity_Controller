"# ESP8266_TempHumidity_Controller" 
Hardware: https://www.openhacks.com/page/productos/id/1906/title/LinkNode-R4-Wifi-relay
Any ESP8266 connected to 4 relays will probably work.

I used the relays to power 3 110VAC US mains outlets.  Seedling Heat pads plugged into two and a fish tank air pump plugged into the third.  The 4th relay is used to switch on/off 5vdc vcc for the two DHT11 sensors.  Power is cut from the DHT11 sensors between readings in order to mionimize electrolysis cortrosion of the sensors while they are in high humidity.

There are 4 web pages supported by the project: 1) Reading/Status 2) Min/Max temp/humidity & frequency of readings settings page 3) SSID/PWD setting page 4) OTA update page

If the esp8266 can't log into the WiFi router AP mode with default 10.0.1.13 SSID/PWD is provided.

