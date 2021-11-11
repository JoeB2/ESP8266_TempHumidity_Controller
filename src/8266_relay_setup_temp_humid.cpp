/*
**  NodeMCU controller of 4 relays: Used to manage temp/humidity for horticulture project
**
**  two DHT11 sensors dht1 & dht2
**  Relays 1, 2, 3 (heater1, heater2, humidifier1)
**  Web page: 10.0.0.113 shows current temps/humidity and state of relays
**  Web page 10.0.0.113/Set_Bounds affords opportunity to set tems/humidity to turn on/off relays
**  Web page 10.0.0.113/Set_Bounds last textbox allows setting frequency for temp/humid sensor checks
**  NOTE: power to sensors is turned off between checks in order to minimize electrolysis of DHT11
**  NOTE: OTA uppdates are implemented: 10.0.0.113/update
**  note: iF EPS8266 Can't log into WiFi stored in SPIFFS/SSIDPWD AP: 10.0.1.13 is implemented
**        10.0.1.13 web page provides opportuntiy to set SSID/PWD; 10.0.1.13/update affords OTA opportunity
**
*/
//#define debug
//#define dbg
#include <Arduino.h>
#include <AsyncElegantOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiType.h>
#include <queue.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <index.h>
#include <string.h>

#define DHTTYPE DHT11
#define HEATER1_pin 16
#define HEATER2_pin 14
#define HUMID1_pin  13


  const byte DNS_PORT  = 53;
  const char *AP_NAME = "T/H Relays-AP: 10.0.1.13";
  const unsigned int wsDelay=7000;  // WebSock send delay and last send times

  bool AP_MODE=false;
  unsigned int lastDHT=millis()-(30*60*1000); // measurments loop millis: fire dht readings on the first loop
  unsigned int lastWS=millis();

IPAddress  // soft AP IP info
          ip_AP(10,0,1,13)
        , ip_STA(10,0,0,113)
        , ip_AP_STA(10,0,1,13)
        , ip_GW(10,0,0,125)
        , ip_AP_GW(10,0,1,13)
        , ip_subNet(255,255,255,128);

// objects
AsyncWebServer server(80);
AsyncWebSocket webSock("/");
DNSServer dnsServer;

DHT dht1(1, DHTTYPE);
DHT dht2(3, DHTTYPE);

typedef struct WifiCreds_t{
      String    SSID;
      String    PWD;
      bool      isDHCP=false;
      IPAddress IP;
      IPAddress GW;
      IPAddress MASK;

      std::string toStr(){
        char s[150];
        sprintf(s, "{\"SSID\":%s,\"PWD\":%s,\"isDHCP\":%s,\"IP\":%s,\"GW\":%s,\"MASK\":%s", SSID.c_str(), PWD.c_str(), isDHCP?"true":"false", IP.toString().c_str(), GW.toString().c_str(), MASK.toString().c_str());
        return(s);
      }
} WifiCreds_t;
WifiCreds_t creds;

typedef struct systemValues_t{   // Scale Information
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
        unsigned int delay;

        std::string toStr(){    // make JSON string
            char c[168];
            int n = sprintf(c, "[{\"t1\":%.1f,\"h1\":%i,\"t2\":%.1f,\"h2\":%i,\"heat1\":%i,\"humid1\":%i,\"heat2\":%i,\"humid2\":%i},{\"t1_on\":%i,\"t1_off\":%i,\"h1_on\":%i,\"h1_off\":%i,\"t2_on\":%i,\"t2_off\":%i,\"delay\":%u}]"
            , t1, h1, t2, h2, heat1, humid1, heat2, humid2, t1_on, t1_off, h1_on, h1_off, t2_on, t2_off, delay);
            return std::string(c, n);
        }
} systemValues_t;
systemValues_t sv;

/******  PROTOTYPES  *******/
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void notFound(AsyncWebServerRequest *request);
void initLocalStruct();
bool wifiConnect(WiFiMode m);
void update_sMsgFromString( const std::string &scaleChar, systemValues_t& p);
std::string valFromJson(const std::string &json, const std::string &element);
bool saveStruct(); // write the local systemValues_t struct string to SPIFFS
void setRelays();
bool initCreds();
void setCreds(const std::string& s);
bool saveCreds();


void setup(){
#ifdef debug
  Serial.begin(115200);
#endif

  SPIFFS.begin();
  AP_MODE = !initCreds() || !wifiConnect(WIFI_STA);

  // Connect to Wi-Fi
  if(AP_MODE){ // can't connect WiFi : go into "get ssid:pwd mode"
    wifiConnect(WIFI_AP);

    webSock.onEvent(onWsEvent);
    server.addHandler(&webSock);

    // Route for root / web page
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    server.on("/"      , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SSIDPWD_HTML);});
    AsyncElegantOTA.begin(&server);
    server.begin();
    yield(); // make sure WDT is happy
  }else{
    initLocalStruct(); // update s_Msg using SPIFFS's JSON if available
    pinMode(12, OUTPUT); // relay : +5v power for the DHT sensors
    pinMode(HUMID1_pin, OUTPUT);
    pinMode(HEATER2_pin, OUTPUT);
    pinMode(HEATER1_pin, OUTPUT);
    dht1.begin(); dht2.begin();

    webSock.onEvent(onWsEvent);
    server.addHandler(&webSock);

    // Route for root / web page
    server.on("/"           , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", INDEX_HTML);});
    server.on("/Set_Bounds" , HTTP_GET, [](AsyncWebServerRequest *request){request->send_P(200, "text/html", SET_BOUNDS_HTML);});

    // Start server
    AsyncElegantOTA.begin(&server);
    server.begin();
  }
}

  void loop(){
    if(AP_MODE)return;

    if(millis()-lastDHT > sv.delay){ // check DHT reading every sv.delay ms
        // note: DHTs are powered down between readings" attempt to minimize electrollysis
        // here we power-up the DHTs for a reading.

        pinMode(1, INPUT_PULLUP);  //GPIO 1 : TX : set use for DHT1 
        pinMode(3, INPUT_PULLUP);  //GPIO 3 : RX : set use for DHT2

        digitalWrite(12, HIGH);
        while(!dht1.read()&&!dht2.read())delay(300);
        delay(100);

        // get temps and humidity
        sv.t1 = dht1.readTemperature(true);
        sv.h1 = dht1.readHumidity();
        sv.t2 = dht2.readTemperature(true);
        sv.h2 = dht2.readHumidity();

        // powerdown DHT sensors
        digitalWrite(12, LOW); // turn DHT power relay off - DHT +5v
        pinMode(1, OUTPUT);digitalWrite(1, LOW); //ground DHT signal wire
        pinMode(3, OUTPUT);digitalWrite(3, LOW); //ground DHT signal wire
        setRelays();

        lastDHT = millis();
    }
    if(millis()-lastWS > wsDelay){// send websock every wsDelay ms
        webSock.textAll(sv.toStr().c_str()); delay(1);
        webSock.cleanupClients();
        lastWS=millis();
    }
  }
  void setRelays(){
    // current state and between on/off temps or above/below absolute on/off boundary
    if(sv.t1<sv.t1_on)sv.heat1=true;if(sv.t1>sv.t1_off)sv.heat1=false;
    if(sv.t2<sv.t2_on)sv.heat2=true;if(sv.t2>sv.t2_off)sv.heat2=false;
    if(sv.h1<sv.h1_on)sv.humid1=true;if(sv.h1>sv.h1_off)sv.humid1=false;
  
    digitalWrite(HEATER1_pin, sv.heat1);
    digitalWrite(HUMID1_pin , sv.humid1);
    digitalWrite(HEATER2_pin, sv.heat2);
  }
  void notFound(AsyncWebServerRequest *request){request->send_P(200, "text/html", INDEX_HTML);}
  void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if(type == WS_EVT_DATA){
      AwsFrameInfo * info = (AwsFrameInfo*)arg;
      if(info->final && !info->index && info->len == len){
        if(info->opcode == WS_TEXT){
          data[len]=0;
          std::string const s=(char *)data;
          if(AP_MODE){
            setCreds(s);
            AP_MODE=saveCreds();
            ESP.restart();
          }else{
                update_sMsgFromString(s, sv);
                saveStruct();
          }
        }
      }
    }
  }
  // load SPIFFS copy of JSON into the local scale's systemValues_t
  void initLocalStruct(){
        File f = SPIFFS.open("/systemValues_t.json", "r");
        if(f){
                const std::string s = f.readString().c_str();
                f.close();
                update_sMsgFromString(s, sv);
        }
#ifdef dbg
Serial.printf("initLocalStruct : sv:%s", sv.toStr().c_str());
#endif       
  }
  // save the local systemValues_t data as JSON string to SPIFFS
  bool saveStruct(){
        File f = SPIFFS.open(F("/systemValues_t.json"), "w");
        if(f){
                f.print(sv.toStr().c_str());
                f.close();
                return true;
        }
        else{return false;}
  }
  // save the local systemValues_t data as JSON string to SPIFFS
  bool saveCreds(){
    File f = SPIFFS.open(F("/creds.json"), "w");
    if(f){
          f.print(creds.toStr().c_str());
          f.close();
  #ifdef dbg
    Serial.printf("saveCreds Success: creds: %s\n", creds.toStr().c_str());Serial.flush();
  #endif
          return true;
    }
    else{
  #ifdef dbg
    Serial.printf("saveCreds FAILED: creds: %s\n", creds.toStr().c_str());Serial.flush();
  #endif
        return(false);
    }
  }
  bool initCreds(){
          File f = SPIFFS.open(F("/creds.json"), "r");
          if(f){
                  std::string s = f.readString().c_str();
                  f.close();
                  setCreds(s);
          }
          else return(false);
  #ifdef dbg
        Serial.printf("\nFailed init creds from SPIFFS: %s\n", creds.toStr().c_str());Serial.flush();
  #endif
    return(true);
  }
  void setCreds(const std::string& s){
    creds.SSID=valFromJson(s, "SSID").c_str();
    creds.PWD=valFromJson(s, "PWD").c_str();
    creds.isDHCP=::atoi(valFromJson(s, "isDHCP").c_str());
    creds.IP.fromString(valFromJson(s, "IP").c_str());
    creds.GW.fromString(valFromJson(s, "GW").c_str());
    creds.MASK.fromString(valFromJson(s, "MASK").c_str());
  }
  // process inbound websock json
  void update_sMsgFromString(const std::string &json, systemValues_t &p){
    p.t1_on =::atof(valFromJson(json, "t1_on").c_str());
    p.t1_off=::atof(valFromJson(json, "t1_off").c_str());
    p.h1_on =::atoi(valFromJson(json, "h1_on").c_str());
    p.h1_off=::atoi(valFromJson(json, "h1_off").c_str());
    p.t2_on =::atof(valFromJson(json, "t2_on").c_str());
    p.t2_off=::atof(valFromJson(json, "t2_off").c_str());
    p.delay =::atoi(valFromJson(json, "delay").c_str());
  }
  // NOTE: Stack Dumps when trying to use <ArduinoJson.h>
  std::string valFromJson(const std::string &json, const std::string &element){
    size_t start, end;
  //         var str= '[{"t1":0,"h1":0,"t2":0},{"t1_on":75,"t1_off":82,"h1_on":75,"h1_off":85,"t2_on":75,"t2_off":80,"delay":7}]';
  //         std""string str= '[{"t1":0,"h1":0,"t2":0},{"t1_on":75,"t1_off":82,"h1_on":75,"h1_off":85,"t2_on":75,"t2_off":80,"delay":7}]';
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
            case WIFI_OFF: return(true);

            case WIFI_STA:
                            WiFi.begin(creds.SSID.c_str(), creds.PWD.c_str());
                            WiFi.channel(2);
                            WiFi.config(ip_STA, ip_GW, ip_subNet);
                            break;

            case WIFI_AP:
                            WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
                            WiFi.softAP(AP_NAME, "");
                            WiFi.begin();
                            break;

            case WIFI_AP_STA:
                            WiFi.channel(2);
                            WiFi.config(ip_AP_STA, ip_AP_GW, ip_subNet);
                            WiFi.enableAP(true);
                            WiFi.softAPConfig(ip_AP, ip_AP_GW, ip_subNet);
                            WiFi.softAP(AP_NAME, "", 3, 0, 4); // default to channel 3,, not hidden, max 4 connctions
                            break;
    }

    unsigned int startup = millis();
    while(WiFi.status() != WL_CONNECTED){
          delay(250);
          Serial.print(".");
          if(millis() - startup >= 5000) break;
    }
#ifdef dbg
Serial.printf("wifiConnect : IP:%s, GW: %s, Mask: %s\n", WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str());
#endif
    return(WiFi.status() == WL_CONNECTED);
  }