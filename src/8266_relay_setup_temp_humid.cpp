/*
**  NodeMCU controller of 4 relays: Used to manage temp/humidity for horticulture project
**
**  two DHT11 sensors dht1 & dht2
**  Relays 1, 2, 3 (heater1, heater2, humidifier1)
**  Web page: 10.0.0.113 shows current temps/humidity and state of relays
**  Web page 10.0.0.113/Set_Bounds affords opportunity to set temps/humidity to turn on/off relays
**  Web page 10.0.0.113/Set_Bounds last textbox allows setting frequency for temp/humid sensor checks
**  NOTE: power to sensors is turned off between checks in order to minimize electrolysis of DHT11
**  NOTE: OTA uppdates are implemented: 10.0.0.113/update
**  note: iF EPS8266 Can't log into WiFi stored in LittleFS/SSIDPWD AP: 10.0.1.13 is implemented
**        10.0.1.13 web page provides opportunity to set SSID/PWD; 10.0.1.13/update affords OTA opportunity
**
*/
#define dbg
#include <Arduino.h>
#include <AsyncElegantOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <index.h>
#include <string.h>
#include <esp8266_peri.h>

/** HEADERS
    #include <Uart_register.h>
    #include "ets_sys.h"
    #include "osapi.h"
    #include "gpio.h"
    #include "os_type.h"
    #include "ip_addr.h"
    #include "mem.h"
    #include "user_interface.h"
    #include "lwip/stats.h"
    #include "espconn.h"
**/

 #define DHT1_PIN 4
 #define DHT2_PIN 5
 #define DHTTYPE DHT11
// P5 r4 s4, T1 r2 s2, T2 r3 s3, H1 r5 s5
 #define DHT_POWER_PIN 12
 #define HEATER1_pin 16
 #define HEATER2_pin 14
 #define HUMID1_pin  13


  const char *AP_NAME = "T/H Relays-AP: 10.0.1.13";
  const unsigned int wsDelay=7000;  // WebSock send delay and last send times

  bool STA_MODE=false;
  unsigned int lastDHT=0; // measurements loop millis: fire dht readings on the first loop

  IPAddress  // soft AP IP info
          ip_AP(10,0,1,13)
        , ip_AP_GW(10,0,1,13)
        , ip_subNet(255,255,255,128);

// objects
AsyncWebServer server(80);
AsyncWebSocket webSock("/");

DHT dht1(DHT1_PIN, DHTTYPE);
DHT dht2(DHT2_PIN, DHTTYPE);

typedef struct WifiCreds_t{
      String    SSID;
      String    PWD;
      bool      isDHCP=false;
      IPAddress IP;
      IPAddress GW;
      IPAddress MASK;

      std::string toStr(){
        char s[150];
        sprintf(s, "{\"SSID\":\"%s\",\"PWD\":\"%s\",\"isDHCP\":%s,\"IP\":\"%s\",\"GW\":\"%s\",\"MASK\":\"%s\"}", SSID.c_str(), PWD.c_str(), isDHCP?"true":"false", IP.toString().c_str(), GW.toString().c_str(), MASK.toString().c_str());
        return(s);
      }
} WifiCreds_t;
WifiCreds_t creds;

typedef struct systemValues_t{   // temp/humid & threshold values
        float t1;
        int   h1;
        float t2;
        int   h2;
        bool  heat1;
        bool  humid1;
        bool  heat2;
        bool  humid2;
        int t1_on;
        int t1_off;
        int h1_on;
        int h1_off;
        int t2_on;
        int t2_off;
        unsigned int delay = 7000;
        unsigned int DHT_Hold = 3;

        std::string toStr(){    // make JSON string
            char c[168];
            int n = sprintf(c, "[{\"t1\":%.1f,\"h1\":%i,\"t2\":%.1f,\"h2\":%i,\"heat1\":%i,\"humid1\":%i,\"heat2\":%i,\"humid2\":%i},{\"t1_on\":%i,\"t1_off\":%i,\"h1_on\":%i,\"h1_off\":%i,\"t2_on\":%i,\"t2_off\":%i,\"delay\":%u,\"DHT_Hold\":3}]"
            , t1, h1, t2, h2, heat1, humid1, heat2, humid2, t1_on, t1_off, h1_on, h1_off, t2_on, t2_off, delay, DHT_Hold);
            return std::string(c, n);
        }
} systemValues_t;
systemValues_t sv;

/******  PROTOTYPES  *******/
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void onWsEvent_Creds(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void notFound(AsyncWebServerRequest *request);
void initLocalStruct();
bool wifiConnect(WiFiMode m);
void update_sMsgFromString( const std::string &scaleChar, systemValues_t& p);
std::string valFromJson(const std::string &json, const std::string &element);
bool saveStruct(); // write the local systemValues_t struct string to LittleFS
void setRelays();
bool initCreds();
void setCreds(const std::string& s);
bool saveCreds();

void setup(){
//  void	wifi_status_led_uninstall	();
#ifdef dbg
  Serial.begin(115200);delay(500);
  Serial.printf("%i :  \nSETUP\n", __LINE__);Serial.flush();
#endif

  LittleFS.begin();

  STA_MODE = initCreds() & wifiConnect(WIFI_STA);

  if(STA_MODE){
#ifdef dbg
  Serial.printf("%i : STA_MODE\n", __LINE__);Serial.flush();
#endif
    initLocalStruct();    // update sv using LittleFs's JSON if available
#ifdef dbg
  Serial.printf("%i : %s : STA_MODE: sv: %s\n", __LINE__, __PRETTY_FUNCTION__, sv.toStr().c_str());Serial.flush();
#endif
    pinMode(HUMID1_pin, OUTPUT);
    pinMode(HEATER1_pin, OUTPUT);
    pinMode(HEATER2_pin, OUTPUT);

    pinMode(DHT_POWER_PIN, OUTPUT);
    digitalWrite(DHT_POWER_PIN, LOW);  // relay : +5v power for the DHT sensors

    webSock.onEvent(onWsEvent);
    server.addHandler(&webSock);

    // Route for root / web page
    server.on("/"           , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", INDEX_HTML);});
    server.on("/Set_Bounds" , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SET_BOUNDS_HTML);});

    // Start server
    AsyncElegantOTA.begin(&server);
    ESP.wdtFeed();
    server.begin();
    dht1.begin();dht2.begin();
  }else{//AP MODE 
#ifdef dbg
  Serial.printf("%i : AP_MODE\n", __LINE__);Serial.flush();
#endif
    wifiConnect(WIFI_AP);
    webSock.onEvent(onWsEvent_Creds);
    server.addHandler(&webSock);

#ifdef dbg
  Serial.printf("%i AP Mode: creds: %s\n", __LINE__, creds.toStr().c_str());Serial.flush();
#endif

    server.on("/"      , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SSIDPWD_HTML);});
    AsyncElegantOTA.begin(&server);
    server.begin();
    ESP.wdtFeed();
  }
}
void loop(){
  delay(100);
#ifdef dbg
  Serial.print(".");Serial.flush();
#endif
    if(!STA_MODE){webSock.textAll(creds.toStr().c_str());delay(3000);ESP.wdtFeed();return;}
    if(lastDHT == 0 || millis()-lastDHT > sv.delay){ // check DHT reading every sv.delay ms
        // note: DHTs are powered down between readings" attempt to minimize electrolysis
        // here we power-up the DHTs for a reading.

        digitalWrite(DHT_POWER_PIN, HIGH); // power up the DHTs
        pinMode(DHT1_PIN, INPUT_PULLUP);
        pinMode(DHT2_PIN, INPUT_PULLUP);

        for(uint32_t i=0; i<sv.DHT_Hold;i++){delay(1000);ESP.wdtFeed();}
          // get temps and humidity
          sv.t1 = dht1.readTemperature(true);
          sv.h1 = dht1.readHumidity();
          sv.t2 = dht2.readTemperature(true);
          sv.h2 = dht2.readHumidity();

        ESP.wdtFeed();
        // powerdown DHT sensors
        digitalWrite(DHT_POWER_PIN, LOW);                      // turn off DHTs' +5vdc
        pinMode(DHT1_PIN, OUTPUT);digitalWrite(DHT1_PIN, LOW); // ground DHT signal wire
        pinMode(DHT2_PIN, OUTPUT);digitalWrite(DHT2_PIN, LOW); // ground DHT signal wire

        setRelays();
        webSock.textAll(sv.toStr().c_str());
        webSock.cleanupClients();
#ifdef dbg
  Serial.printf("\n%i : Loop: sv: %s\n", __LINE__, sv.toStr().c_str());Serial.flush();
#endif
        lastDHT = millis();
    }
  }
  void setRelays(){
    // current state and between on/off temps or above/below absolute on/off boundary
    if(sv.t1<sv.t1_on)sv.heat1=true;
      if(sv.t1>sv.t1_off)sv.heat1=false;

    if(sv.t2<sv.t2_on)sv.heat2=true;
      if(sv.t2>sv.t2_off)sv.heat2=false;

    if(sv.h1<sv.h1_on)sv.humid1=true;
      if(sv.h1>sv.h1_off)sv.humid1=false;
  
    digitalWrite(HEATER1_pin, sv.heat1);
    digitalWrite(HUMID1_pin , sv.humid1);
    digitalWrite(HEATER2_pin, sv.heat2);
  }
  void notFound(AsyncWebServerRequest *request){request->send_P(200, "text/html", INDEX_HTML);}
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type==WS_EVT_CONNECT){client->text(sv.toStr().c_str());return;}
    if(type==WS_EVT_DISCONNECT){return;}
    
    if(type == WS_EVT_DATA){
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && !info->index && info->len == len){
        if(info->opcode == WS_TEXT){
          data[len]=0;
          std::string const s=(char *)data;
#ifdef dbg
  Serial.printf("\n%i : WS: data: %s, %i\n", __LINE__, s.c_str(), s.length());Serial.flush();
#endif
            if(s != "Connect"){
              update_sMsgFromString(s, sv);
              saveStruct();
            }
          client->text(sv.toStr().c_str());
        }
      }
    }
  }
  void onWsEvent_Creds(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type==WS_EVT_CONNECT){
      client->text(creds.toStr().c_str());
#ifdef dbg
  Serial.printf("\n%i : WS_Creds: data: %s, %i\n", __LINE__, creds.toStr().c_str(), creds.toStr().length());Serial.flush();
#endif
      return;
    }
    if(type==WS_EVT_DISCONNECT){return;}
    if(type == WS_EVT_DATA){
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && !info->index && info->len == len){
        if(info->opcode == WS_TEXT){
          data[len]=0;
          std::string const s=(char *)data;
          if(s == "Connect"){
            webSock.textAll(creds.toStr().c_str());
            return;
          }
#ifdef dbg
  Serial.printf("\n%i : WS_Creds: data: %s, %i\n", __LINE__, s.c_str(), s.length());Serial.flush();
#endif
            setCreds(s);
            saveCreds();
            ESP.restart();
        }
      }
    }
  }
  // load LittleFS copy of JSON into the local scale's systemValues_t
  void initLocalStruct(){
        File f = LittleFS.open("/systemValues_t.json", "r");
        if(f){
                const std::string s = f.readString().c_str();
                f.close();
                update_sMsgFromString(s, sv);
        }
#ifdef dbg
  Serial.printf("%i : initLocalStruct : sv:%s\n", __LINE__, sv.toStr().c_str());Serial.flush();
#endif       
  }
  bool saveStruct(){
        File f = LittleFS.open(F("/systemValues_t.json"), "w");
        if(f){
                f.print(sv.toStr().c_str());
                f.close();
                return true;
        }
        else{return false;}
  }
  bool saveCreds(){
    File f = LittleFS.open(F("/creds.json"), "w");
    if(f){
          f.print(creds.toStr().c_str());
          f.close();
  #ifdef dbg
    Serial.printf("%i : saveCreds Success: creds: %s\n", __LINE__, creds.toStr().c_str());Serial.flush();
  #endif
          return true;
    }
    else{
  #ifdef dbg
    Serial.printf("%i : saveCreds FAILED: creds: %s\n", __LINE__, creds.toStr().c_str());Serial.flush();
  #endif
        return(false);
    }
  }
  bool initCreds(){
          File f = LittleFS.open(F("/creds.json"), "r");
          if(f){
                  std::string s = f.readString().c_str();
                  f.close();
                  setCreds(s);
                  return(true);
          }
  #ifdef dbg
    Serial.printf("\n%i : Failed init creds from LittleFS: %s\n", __LINE__, creds.toStr().c_str());Serial.flush();
  #endif
    return(false);
  }
  void setCreds(const std::string& s){
    creds.SSID=valFromJson(s, "SSID").c_str();
    creds.PWD=valFromJson(s, "PWD").c_str();
    creds.isDHCP=::atoi(valFromJson(s, "isDHCP").c_str());
    creds.IP.fromString(valFromJson(s, "IP").c_str());
    creds.GW.fromString(valFromJson(s, "GW").c_str());
    creds.MASK.fromString(valFromJson(s, "MASK").c_str());
  }
  void update_sMsgFromString(const std::string &json, systemValues_t &p){
#ifdef dbg
  Serial.printf("%i : update_sv\n", __LINE__);Serial.flush();
#endif
    p.t1_on =::atof(valFromJson(json, "t1_on").c_str());
    p.t1_off=::atof(valFromJson(json, "t1_off").c_str());
    p.h1_on =::atoi(valFromJson(json, "h1_on").c_str());
    p.h1_off=::atoi(valFromJson(json, "h1_off").c_str());
    p.t2_on =::atof(valFromJson(json, "t2_on").c_str());
    p.t2_off=::atof(valFromJson(json, "t2_off").c_str());
    p.delay =::atoi(valFromJson(json, "delay").c_str());
    p.DHT_Hold = ::atoi(valFromJson(json, "DHT_Hold").c_str());
  }
  std::string valFromJson(const std::string &json, const std::string &element){// got stack dumps with ArduinoJson
    size_t start, end;
  //         var str= '[{"t1":0,"h1":0,"t2":0},{"t1_on":75,"t1_off":82,"h1_on":75,"h1_off":85,"t2_on":75,"t2_off":80,"delay":7}]';
  //         std::string str= '[{"t1":0,"h1":0,"t2":0},{"t1_on":75,"t1_off":82,"h1_on":75,"h1_off":85,"t2_on":75,"t2_off":80,"delay":7}]';
    start = json.find(element);
    start = json.find(":", start)+1;
    if(json.substr(start,1) =="\"")start++;
    end  = json.find_first_of(",]}\"", start);
    return(json.substr(start, end-start));
  }
  bool wifiConnect(WiFiMode m){
    WiFi.disconnect();
    WiFi.softAPdisconnect();
    WiFi.mode(m);// WIFI_AP_STA,WIFI_AP; WIFI_STA;
    
    switch(m){
            case WIFI_STA:
                            WiFi.begin(creds.SSID.c_str(), creds.PWD.c_str());
                            WiFi.channel(2);
                            if(!creds.isDHCP)
                              WiFi.config(creds.IP, creds.GW, creds.MASK);
                            break;
            case WIFI_AP:
                            WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
                            WiFi.softAP(AP_NAME, "");
                            WiFi.begin();
                            break;

            case WIFI_AP_STA: break;
            case WIFI_OFF:    break;
    }
    unsigned int startup = millis();
    while(WiFi.status() != WL_CONNECTED){
          delay(250);
#ifdef dbg          
          Serial.print(".");
#endif
          if(millis() - startup >= 5000) break;
    }
#ifdef dbg
  Serial.println("");Serial.flush();
  Serial.printf("%i : wifiConnect : IP:%s, GW: %s, Mask: %s\n", __LINE__, WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str());Serial.flush();
#endif
    return(WiFi.status() == WL_CONNECTED);
  }
