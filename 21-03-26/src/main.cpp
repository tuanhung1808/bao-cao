#include <Arduino.h>
#include "SPIFFS.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Mount SPIFFS
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS Mount Failed!");
    return;
  }

  Serial.println("SPIFFS OK");

  // Mở file
  File file = SPIFFS.open("/giao việc về mqtt.txt");

  if (!file) {
    Serial.println("Không mở được file!");
    return;
  }

  Serial.println("Đọc nội dung file:");

  // Đọc từng byte
  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();
}

void loop() {
}