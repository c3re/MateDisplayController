# MateDisplayController

This Sketch opens a UDP port and listens for TMP2.NET on this port.
WiFi network is configured via WiFiManager.
Firmware can be updated OTA.

For now, you have to set the number of pixels and the pin you connect your LEDs to by copying or renaming `Config.h.example` to `Config.h`.

You need: 

- ESP8266WiFi.h
- ESP8266mDNS.h
- WiFiUdp.h
- ArduinoOTA.h
- Adafruit_NeoPixel.h
- DNSServer.h
- ESP8266WebServer.h
- WiFiManager.h

All libraries are available through Arduino IDE package manager.
