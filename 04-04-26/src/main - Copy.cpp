#include <WiFi.h>      // thư viện WiFi
#include "SPIFFS.h"    // filesystem trong flash

#define INI_FILE "/wifi.ini"   // tên file lưu config

/* ====================== */
/* KHỞI TẠO SPIFFS        */
/* ====================== */
void initFS()
{
    // mount filesystem
    if(!SPIFFS.begin(true))   // true = format nếu lỗi
    {
        Serial.println("SPIFFS mount FAIL");
        while(1);
    }

    Serial.println("SPIFFS READY");
}


/* ====================== */
/* GHI FILE CONFIG        */
/* ====================== */
void saveWifi(const char* ssid, const char* password)
{
    // mở file mode "w"
    // w = write + xoá nội dung cũ
    File file = SPIFFS.open(INI_FILE, "w");

    if(!file)                 // nếu mở thất bại
    {
        Serial.println("Open file FAIL");
        return;
    }

    /* ---- ghi nội dung ---- */

    file.println("[WIFI]");   // section INI

    file.print("ssid=");      // ghi key
    file.println(ssid);       // ghi value

    file.print("password=");
    file.println(password);

    file.close();             // BẮT BUỘC đóng file
                              // nếu không flash chưa ghi thật

    Serial.println("WiFi saved to flash");
}


/* ====================== */
/* ĐỌC FILE ĐỂ KIỂM TRA   */
/* ====================== */
void readFile()
{
    File file = SPIFFS.open(INI_FILE, "r");

    if(!file)
    {
        Serial.println("Read FAIL");
        return;
    }

    Serial.println("---- FILE CONTENT ----");

    // đọc từng byte và in ra Serial
    while(file.available())
    {
        Serial.write(file.read());
    }

    file.close();
}


/* ====================== */
/* SETUP                  */
/* ====================== */
void setup()
{
    Serial.begin(115200);

    initFS();   // mount flash

    // giả lập user nhập WiFi mới
    saveWifi("MyWifi","12345678");

    // đọc lại để kiểm tra
    readFile();
}


/* ====================== */
void loop()
{
}