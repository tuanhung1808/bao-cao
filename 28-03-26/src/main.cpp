#include <WiFi.h>
#include "SPIFFS.h"

String ssid;
String password;

void initFS()
{
    if(!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    Serial.println("SPIFFS OK");
}

void readConfig()
{
    File file = SPIFFS.open("/wifi.ini");

    if(!file)
    {
        Serial.println("Open wifi.ini FAIL");
        return;
    }

    Serial.println("Reading wifi.ini ...");

    while(file.available())
    {
        String line = file.readStringUntil('\n');

        line.trim();   // bỏ khoảng trắng

        if(line.startsWith("ssid"))
        {
            ssid = line.substring(line.indexOf("=")+1);
            ssid.trim();
        }

        if(line.startsWith("password"))
        {
            password = line.substring(line.indexOf("=")+1);
            password.trim();
        }
    }

    file.close();
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    initFS();
    readConfig();

        /* ===== IN RA SERIAL ===== */
    Serial.print("SSID: ");
    Serial.println(ssid);

    Serial.print("PASSWORD: ");
    Serial.println(password);
}

void loop(){}