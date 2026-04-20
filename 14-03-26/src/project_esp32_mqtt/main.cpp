#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ssid = "TuanDung";
const char* password = "0913583737";

const char* mqtt_server = "449b784be10a4cc9b33005afc8231712.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;

const char* mqtt_user = "ESP32_MQTT";
const char* mqtt_pass = "Abc@1234";

#define LED1 14
#define LED2 25

WiFiClientSecure espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
 
  String message;

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  Serial.println(message);

  if (String(topic) == "esp32/led1") {

    if (message == "1") {
      digitalWrite(LED1, HIGH);
      client.publish("esp32/led1/status","ON");
    }
    else {
      digitalWrite(LED1, LOW);
      client.publish("esp32/led1/status","OFF");
    }

  }

  if (String(topic) == "esp32/led2") {

    if (message == "1") {
      digitalWrite(LED2, HIGH);
      client.publish("esp32/led2/status","ON");
    }
    else {
      digitalWrite(LED2, LOW);
      client.publish("esp32/led2/status","OFF");
    }

  }
}

void reconnect() {

  while (!client.connected()) {

    Serial.print("Connecting MQTT...");

    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {

      Serial.println("connected");

      client.subscribe("esp32/led1");
      client.subscribe("esp32/led2");

      client.publish("esp32/led1/status","OFF");
      client.publish("esp32/led2/status","OFF");

    } else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 2 seconds");

      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  Serial.println("Connecting WiFi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");

  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
}