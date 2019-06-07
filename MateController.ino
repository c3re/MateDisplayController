/*
 * MateDisplayController
 * 
 * nutcase 2019-06-06
 * 
 */
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

// WiFiManager stuff
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "Config.h"

#ifndef PIN
  #define PIN            D4
#endif
#ifndef NUMPIXELS
  #define NUMPIXELS      20
#endif

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

WiFiUDP udp;
unsigned int tpm2NetPort = 65506; // TPM2.NET port


/* 
 *  TPM2.NET stuff
 */
const int PACKET_SIZE = 1357;
byte packetBuffer[PACKET_SIZE]; //buffer to hold incoming and outgoing packets
int led_index = 0;

void setup_ota() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void setup_pixels() {
  pixels.clear();
  // TODO: add some init/dummy sequence?
  pixels.begin();  
}

void setup_udp() {
  Serial.println("Open UDP port");
  udp.begin(tpm2NetPort);
  Serial.print("Opened TPM2.NET port: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(udp.localPort());
}

void setup_wifi() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("WiFiManager");
  Serial.print(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  setup_wifi();
  setup_ota();
  setup_pixels();
  setup_udp();
  Serial.println("Ready");
}

/*
 * Main Loop Stuff
 */

void tpm2NetHandle() {
 /* TPM2.NET Stuff
 *  taken from https://hackaday.io/project/25851-pixel-led-wifi-controller-tpm2net
 */
  int cb = udp.parsePacket();
  if (!cb) {
    // nothing
  } else {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, PACKET_SIZE); // read the packet into the buffer
    if (cb >= 6 && packetBuffer[0] == 0x9C)  { // header identifier (packet start)
      byte blocktype = packetBuffer[1]; // block type (0xDA)
      unsigned int framelength = ((unsigned int)packetBuffer[2] << 8) | (unsigned int)packetBuffer[3]; // frame length (0x0069) = 105 leds
      byte packagenum = packetBuffer[4];   // packet number 0-255 0x00 = no frame split (0x01)
      byte numpackages = packetBuffer[5];   // total packets 1-255 (0x01)
                   
      if (blocktype == 0xDA) {
        // data frame ...
        // Serial.println("command");
        
        int packetindex;
        
        if (cb >= framelength + 7 && packetBuffer[6 + framelength] == 0x36) { 
          // header end (packet stop)
          // Serial.println("s:");
          int i = 0;
          packetindex = 6;
          if(packagenum == 1) {
            led_index =0;
          }
          while(packetindex < (framelength + 6)) {
            int r =((int)packetBuffer[packetindex]);
            int g =((int)packetBuffer[packetindex+1]);
            int b =((int)packetBuffer[packetindex+2]);
            pixels.setPixelColor(led_index, pixels.Color(r,g,b));
            led_index++;         
            packetindex +=3;
          }
        }
      }
    
      if((packagenum == numpackages) && (led_index== NUMPIXELS)) {
        pixels.show();
        led_index==0;
      }  
    }
  } 
}

void loop() {
  ArduinoOTA.handle();
  tpm2NetHandle();
}
