#include <WiFi.h>
#include "SPIFFS.h"
#include "IniFile.h"

bool getIniValue(const char* filename,
                 const char* section,
                 const char* key,
                 char* output,
                 size_t maxLen)
{
    File file = SPIFFS.open(filename, "r");
    if(!file) return false;

    String currentSection = "";

    while(file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        if(line.length()==0) continue;
        if(line.startsWith(";")) continue;

        // section
        if(line.startsWith("[") && line.endsWith("]"))
        {
            currentSection =
                line.substring(1,line.length()-1);
            continue;
        }

        if(currentSection != section)
            continue;

        int eq = line.indexOf('=');
        if(eq < 0) continue;

        String k = line.substring(0,eq);
        String v = line.substring(eq+1);

        k.trim();
        v.trim();

        if(k == key)
        {
            strncpy(output, v.c_str(), maxLen-1);
            output[maxLen-1] = '\0';

            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

char ssid[32];
char password[64];
char giatristr[10];

void initFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS FAIL");
        while(1);
    }

    Serial.println("SPIFFS OK");
}

void read()
{

getIniValue("/wifi.ini","WIFI","ssid",ssid,sizeof(ssid));
getIniValue("/wifi.ini","WIFI","password",password,sizeof(password));
getIniValue("/wifi.ini","test","giatri",giatristr,sizeof(giatristr));
int giatri = atoi(giatristr);

    Serial.println(ssid);
    Serial.println(password);
    Serial.println(giatri);


}

void setup()
{
    Serial.begin(115200);

    initFS();
    read();
}

void loop(){}