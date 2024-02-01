#include <WiFi.h>
#include "WiFiClientSecure.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"

WiFiClient wifiClient;
Adafruit_MQTT_Client mqttClient(&wifiClient, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_MQTT_Publish statusFeed(&mqttClient, STATUS_FEED);
Adafruit_MQTT_Subscribe commandFeed(&mqttClient, COMMAND_FEED, MQTT_QOS_1);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  blinkLED(1);

  connectToNetwork(connectWiFi, "WiFi");
  setupMQTT();
  blinkLED(3);
}

void setup1() {
  // Setup tasks for core1, if any
}

void loop() {
  // Network communication tasks
  maintainConnections();
  mqttClient.processPackets(2500);

  // Handle incoming MQTT commands and push to core1
  if (commandFeed.available()) {
    char* commandData = (char*)commandFeed.lastread;
    while (!rp2040.fifo.push_nb((uint32_t)commandData)) {
      // Retry or handle full FIFO
      delay(10);
    }
  }
}

void loop1() {
  uint32_t receivedData;
  while (true) {
    if (rp2040.fifo.pop_nb(&receivedData)) {
      char* commandData = (char*)receivedData;
      executeCommand(commandData);
    }
  }
}

void executeCommand(char* command) {
  // Implement command execution logic
}

void blinkLED(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, LOW); delay(100);
    digitalWrite(LED_BUILTIN, HIGH); delay(200);
    digitalWrite(LED_BUILTIN, LOW); delay(100);
  }
}

void connectToNetwork(void (*connectFunction)(), const char *networkType) {
  if (!(*connectFunction)()) {
    Serial.println(networkType + String(" connection failed. Entering error recovery."));
    errorRecovery();
  }
}

bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  for (int retries = 3; retries > 0; retries--) {
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(5000);
    blinkLED(2);
  }
  return false;
}

void setupMQTT() {
  commandFeed.setCallback(commandFeedCallback);
  mqttClient.subscribe(&commandFeed);
  if (!connectMqtt()) {
    Serial.println("MQTT setup failed. Entering error recovery.");
    errorRecovery();
  }
  String welcomeMsg = createStatusMessage();
  statusFeed.publish(welcomeMsg.c_str());
}

bool connectMqtt() {
  if (mqttClient.connected()) return true;

  for (int retries = 3; retries > 0; retries--) {
    if (mqttClient.connect() == 0) return true;
    mqttClient.disconnect();
    delay(5000);
    blinkLED(2);
  }
  return false;
}

void maintainConnections() {
  if (!mqttClient.connected() && !connectMqtt()) {
    Serial.println("MQTT reconnect failed. Entering error recovery.");
    errorRecovery();
  }
}

String createStatusMessage() {
  return "Device " + String(BOARD_ID) + " connected. IP " + WiFi.localIP().toString() + ", Mac " + WiFi.macAddress() + ".";
}

void commandFeedCallback(char *data, uint16_t len) {
  // Implement command processing logic
}

void errorRecovery() {
  blinkLED(10);
  // Implement error recovery logic, like resetting the device
}