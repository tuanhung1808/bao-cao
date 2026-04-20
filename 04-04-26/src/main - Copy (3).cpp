#include <WiFi.h>
#include "SPIFFS.h"

#define INI_FILE "/wifi.ini"

/* ================= SPIFFS ================= */
void initFS()
{
    if(!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS FAIL");
        while(1);
    }

    Serial.println("SPIFFS READY");
}

/* ================= SAVE FILE ================= */
void saveWifi(const char* ssid,const char* pass)
{
    File file = SPIFFS.open(INI_FILE,"w");

    if(!file)
    {
        Serial.println("Write FAIL");
        return;
    }

    file.println("[WIFI]");
    file.print("ssid=");
    file.println(ssid);

    file.print("password=");
    file.println(pass);

    file.close();

    Serial.println("Saved wifi.ini");
}

/* ================= SHOW FILE ================= */
void showFile()
{
    File file = SPIFFS.open(INI_FILE,"r");

    if(!file)
    {
        Serial.println("No file");
        return;
    }

    Serial.println("----- FILE -----");

    while(file.available())
        Serial.write(file.read());

    file.close();
}

/* ================= SERIAL COMMAND ================= */
void checkSerial()
{
    if(!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n'); // đọc 1 dòng
    cmd.trim();

    /* -------- SET WIFI ---------- */
    if(cmd.startsWith("SET WIFI"))
    {
        // format:
        // SET WIFI ssid password

        int s1 = cmd.indexOf(' ',9); // tìm khoảng trắng

        if(s1 < 0)
        {
            Serial.println("Syntax error");
            return;
        }

        String ssid = cmd.substring(9,s1);
        String pass = cmd.substring(s1+1);

        saveWifi(ssid.c_str(),pass.c_str());
    }

    /* -------- SHOW FILE ---------- */
    else if(cmd == "SHOW")
    {
        showFile();
    }

    else
    {
        Serial.println("Unknown command");
    }
}

/* ================= SETUP ================= */
void setup()
{
    Serial.begin(115200);

    initFS();

    Serial.println("READY COMMAND MODE");
}

/* ================= LOOP ================= */
void loop()
{
    checkSerial();   // luôn kiểm tra lệnh serial
}