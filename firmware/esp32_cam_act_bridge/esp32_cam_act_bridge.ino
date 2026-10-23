#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Arduino.h>
#include <HardwareSerial.h>
#include "secrets.h"

// =================================================
// WIFI CONFIG
// =================================================
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// =================================================
// UDP DISCOVERY
// =================================================
WiFiUDP udp;
const unsigned int UDP_PORT = 4210;
const char* PING_MSG = "AMVISH_PING";
const char* PONG_MSG = "AMVISH_PONG";

// =================================================
// HTTP SERVER
// =================================================
WebServer server(80);

// =================================================
// UART TO ARDUINO
// =================================================
HardwareSerial Serial_ard(1);
#define ESP_TX_PIN 15    // GPIO15 → Arduino RX
#define ESP_RX_PIN -1    // RX disabled
#define ARD_BAUD 9600

// =================================================
// HTTP ROOT (STATUS)
// =================================================
void handleRoot() {
  server.send(200, "text/plain", "ESP32-CAM ONLINE | ACT BRIDGE READY");
}

// =================================================
// HTTP /ACT ENDPOINT
// =================================================
void handleACT() {
  Serial.println("[HTTP] /ACT HIT");

  if (!server.hasArg("plain")) {
    Serial.println("[HTTP] NO BODY");
    server.send(400, "text/plain", "NO BODY");
    return;
  }

  String body = server.arg("plain");
  body.trim();

  Serial.print("[HTTP] /ACT body: ");
  Serial.println(body);

  // ---- simple parse (unchanged logic) ----
  int idx = body.indexOf("\"act\"");
  if (idx == -1) {
    server.send(400, "text/plain", "INVALID JSON");
    return;
  }

  int start = body.indexOf("\"", idx + 5);
  int end   = body.indexOf("\"", start + 1);
  if (start == -1 || end == -1) {
    server.send(400, "text/plain", "PARSE ERROR");
    return;
  }

  String act = body.substring(start + 1, end);
  act.trim();

  String uartMsg = "ACT:" + act + "\n";
  Serial_ard.print(uartMsg);

  Serial.print("[UART] → Arduino: ");
  Serial.println(uartMsg);

  server.send(200, "text/plain", "ACT FORWARDED");
}


// =================================================
// UDP DISCOVERY HANDLER
// =================================================
void handleUdpDiscovery() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return;

  char buf[32];
  int len = udp.read(buf, sizeof(buf) - 1);
  if (len <= 0) return;
  buf[len] = '\0';

  if (strcmp(buf, PING_MSG) == 0) {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*)PONG_MSG, strlen(PONG_MSG));
    udp.endPacket();

    Serial.print("[UDP] Ping → Pong sent to ");
    Serial.println(udp.remoteIP());
  }
}

// =================================================
// SETUP
// =================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n[ESP32] Booting ACT Bridge");

  // ---- UART to Arduino ----
  Serial_ard.begin(ARD_BAUD, SERIAL_8N1, ESP_RX_PIN, ESP_TX_PIN);
  Serial.println("[ESP32] UART → Arduino ready (9600 baud)");

  // ---- WiFi ----
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

  Serial.println("\n[ESP32] WiFi connected");
  Serial.print("[ESP32] IP: ");
  Serial.println(WiFi.localIP());

  // ---- UDP ----
  udp.begin(UDP_PORT);
  Serial.println("[ESP32] UDP discovery ready");

  // ---- HTTP ----
  server.on("/", handleRoot);
  server.on("/ACT", HTTP_POST, handleACT);
  server.begin();

  Serial.println("[ESP32] HTTP server started");
}

// =================================================
// LOOP
// =================================================
void loop() {
  server.handleClient();
  handleUdpDiscovery();
}
