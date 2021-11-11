"# ESP8266_TempHumidity_Controller" 
Hardware: https://www.openhacks.com/page/productos/id/1906/title/LinkNode-R4-Wifi-relay
Any ESP8266 connected to 4 relays will probably work.

I used the relays to power 3 110VAC US mains outlets: 1) seedling #1 heat pad, 2) seedling #2 heat pad, 3) seedling #1 bin humidifier. The 4th relay is used to switch on/off +5vdc vcc for the two DHT11 sensors. DHT11 power is cut between readings in order to minimize electrolysis corrosion while the DHT11 sensors are in high humidity.

4 web pages are hosted: 1) Reading/Status html 2) settings html: Min & Max temp/humidity for switching on/off the relays & a sensor reading delay time 3) WiFi Credential page: SSID/PWD, IP, GW, Mask, DHCP page 4) OTA update page.  NOTE: Websock is used to update web -pages and to send back settings to the MCU.

If the esp8266 can't log into the local WiFi : Router AP Mode http://10.0.1.13 Credential page is hosted.  http://IP/update is uised for OTA.
