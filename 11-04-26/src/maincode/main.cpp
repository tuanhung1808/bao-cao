#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "LittleFS.h"
#include "IniFile.h"

/* ================= FILE ================= */

#define WIFI_FILE "/wifi.ini"
#define CONFIG_FILE "/config.ini"

/* ================= DHT ================= */

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

/* ================= MQTT DATA ================= */

char hive_server[128];
char hive_user[64];
char hive_pass[64];

char emqx_server[128];
char emqx_user[64];
char emqx_pass[64];

char flespi_server[128];
char flespi_token[128];

/* ================= TLS ================= */

WiFiClientSecure netHive;
WiFiClientSecure netEmqx;
WiFiClientSecure netFlespi;

PubSubClient mqttHive(netHive);
PubSubClient mqttEmqx(netEmqx);
PubSubClient mqttFlespi(netFlespi);

/* ========================================================= */
/* ================= LITTLEFS =============================== */
/* ========================================================= */

void initFS()
{
    if(!LittleFS.begin(true))
    {
        Serial.println("LittleFS FAIL");
        while(1);
    }
    Serial.println("LittleFS READY");
}

/* ========================================================= */
/* ================= SAFE WIFI INIT ========================= */
/* ========================================================= */

void initWiFiDriver()
{
    WiFi.mode(WIFI_STA);

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);

    WiFi.disconnect(true);   // chạy 1 lần duy nhất
    delay(1000);
}

/* ========================================================= */
/* ================= AUTO WIFI ============================== */
/* ========================================================= */

void autoConnectWiFi()
{
    char ssid[32];
    char pass[32];
    char section[16];

    int n = 1;

    while(WiFi.status()!=WL_CONNECTED)
    {
        sprintf(section,"WIFI%d",n);

        if(!getIniValue(WIFI_FILE,
                        section,
                        "ssid",
                        ssid,
                        sizeof(ssid)))
        {
            Serial.println("Restart WiFi list...");
            n = 1;
            continue;
        }

        getIniValue(WIFI_FILE,
                    section,
                    "password",
                    pass,
                    sizeof(pass));

        Serial.printf("Connecting -> %s\n",ssid);

        WiFi.begin(ssid,pass);

        unsigned long t = millis();

        while(millis()-t < 8000)
        {
            if(WiFi.status()==WL_CONNECTED)
                break;

            delay(200);
        }

        if(WiFi.status()==WL_CONNECTED)
        {
            Serial.println("\nWiFi CONNECTED");
            return;
        }

        Serial.println("Fail → next WiFi");

        WiFi.disconnect();   // FIX CRASH
        delay(800);

        n++;
    }
}

/* ========================================================= */
/* ================= LOAD CONFIG ============================ */
/* ========================================================= */

void loadConfig()
{
    Serial.println("Loading config.ini");

    getIniValue(CONFIG_FILE,"HIVEMQ","hive_server",hive_server,sizeof(hive_server));
    getIniValue(CONFIG_FILE,"HIVEMQ","hive_user",hive_user,sizeof(hive_user));
    getIniValue(CONFIG_FILE,"HIVEMQ","hive_pass",hive_pass,sizeof(hive_pass));

    getIniValue(CONFIG_FILE,"EMQX","emqx_server",emqx_server,sizeof(emqx_server));
    getIniValue(CONFIG_FILE,"EMQX","emqx_user",emqx_user,sizeof(emqx_user));
    getIniValue(CONFIG_FILE,"EMQX","emqx_pass",emqx_pass,sizeof(emqx_pass));

    getIniValue(CONFIG_FILE,"FLESPI","flespi_server",flespi_server,sizeof(flespi_server));
    getIniValue(CONFIG_FILE,"FLESPI","flespi_token",flespi_token,sizeof(flespi_token));

    Serial.println("CONFIG LOADED");
}

/* ========================================================= */
/* ================= MQTT =================================== */
/* ========================================================= */

void setupMQTT()
{
    netHive.setInsecure();
    netEmqx.setInsecure();
    netFlespi.setInsecure();

    mqttHive.setServer(hive_server,8883);
    mqttEmqx.setServer(emqx_server,8883);
    mqttFlespi.setServer(flespi_server,8883);
}

void connectMQTT()
{
    if(!mqttHive.connected())
        mqttHive.connect("ESP32_HIVE",hive_user,hive_pass);

    if(!mqttEmqx.connected())
        mqttEmqx.connect("ESP32_EMQX",emqx_user,emqx_pass);

    if(!mqttFlespi.connected())
        mqttFlespi.connect("ESP32_FLESPI",flespi_token,"");
}

/* ========================================================= */
/* ================= DHT READ =============================== */
/* ========================================================= */

float tempCache=0;
float humiCache=0;
unsigned long lastRead=0;

void readDHT()
{
    float h=dht.readHumidity();
    float t=dht.readTemperature();

    if(!isnan(t)&&!isnan(h))
    {
        tempCache=t;
        humiCache=h;
    }
}

/* ========================================================= */
/* ================= SETUP ================================= */
/* ========================================================= */

void setup()
{
    Serial.begin(115200);

    dht.begin();

    initFS();

    initWiFiDriver();   
    autoConnectWiFi();

    loadConfig();
    setupMQTT();
}

/* ========================================================= */
/* ================= LOOP ================================== */
/* ========================================================= */

unsigned long lastPub=0;

void loop()
{
    connectMQTT();

    mqttHive.loop();
    mqttEmqx.loop();
    mqttFlespi.loop();

    if(millis()-lastRead>2000)
    {
        lastRead=millis();
        readDHT();
    }

    if(millis()-lastPub>3000)
    {
        lastPub=millis();

        char payload[80];

        sprintf(payload,
                "{\"temp\":%.2f,\"humi\":%.2f}",
                tempCache,humiCache);

        mqttHive.publish("esp32/dht",payload);
        mqttEmqx.publish("esp32/dht",payload);
        mqttFlespi.publish("esp32/dht",payload);

        Serial.println(payload);
    }
}