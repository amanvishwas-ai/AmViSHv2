//Serial_ard.println("ESP32_UART_READY");
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include "secrets.h"


// -------- STATE --------
bool face_present = false;

// -------- LAPTOP DISCOVERY --------
IPAddress laptopIP;
bool laptopKnown = false;
const int port = 8081;

// -------- UART (TO ARDUINO) --------
HardwareSerial Serial_ard(1);

// TX only... RX pin set to -1 (disabled)
#define ESP_TX_PIN 15
#define ESP_RX_PIN -1

// -------- NETWORK --------
WiFiUDP udp;
WebServer server(80);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const unsigned int UDP_PORT = 4210;
const char* PING_MSG = "AMVISH_PING";
const char* PONG_MSG = "AMVISH_PONG";

// ------------------------------------------------
// HTTP ROOT
// ------------------------------------------------
void handleRoot() {
  server.send(200, "text/plain", "ESP32-CAM ONLINE (NO CAMERA INIT)");
}

void handleStatus() {
  server.send(
    200,
    "application/json",
    "{\"device\":\"ESP32-CAM\",\"camera\":\"disabled\",\"ok\":true}"
  );
}

// ------------------------------------------------
// SEND EVENT TO LAPTOP (HTTP READY)
// ------------------------------------------------
void sendEvent(int slot, const char* event) {
  if (!laptopKnown) return;

  HTTPClient http;
  String url = "http://" + laptopIP.toString() + ":" + String(port) + "/face_event";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"face_slot\":" + String(slot) + ",";
  payload += "\"event\":\"" + String(event) + "\"}";

  http.POST(payload);
  http.end();
}

// ------------------------------------------------
void setup() {
  Serial.begin(115200);

  // UART to Arduino — DO NOT TOUCH
  Serial_ard.begin(
    9600,
    SERIAL_8N1,
    ESP_RX_PIN,
    ESP_TX_PIN
  );

  Serial_ard.println("WIFI_CONNECTING");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 5) {
    delay(1000);
    tries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  Serial_ard.println("WIFI_CONNECTED");

  // UDP discovery
  udp.begin(UDP_PORT);
  Serial_ard.println("LAPTOP_CONNECTING");

  // HTTP server
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();
}

// ------------------------------------------------
void loop() {
  // Keep HTTP server alive
  server.handleClient();

  // -------- UDP DISCOVERY --------
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buffer[64];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len > 0) buffer[len] = '\0';

    if (String(buffer) == PING_MSG) {
      laptopIP = udp.remoteIP();
      laptopKnown = true;

      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.print(PONG_MSG);
      udp.endPacket();

      Serial_ard.println("LAPTOP_CONNECTED");
    }
  }

  // -------- NO CAMERA, NO FACE LOGIC --------
  // sendEvent(...) intentionally unused
}
