#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

// --- Wi-Fi Credentials ---
const char* ssid = "Nothing Phone (2)";
const char* password = "12345678";
WiFiServer server(8080); // Create a server on port 8080

// --- LoRa Pin Definitions ---
#define LORA_NSS    5
#define LORA_RST    14
#define LORA_DIO0   2

// Store last received LoRa data
String loraDataFromTransmitter = "No data yet";

void setup() {
  Serial.begin(115200);

  // --- Connect to Wi-Fi ---
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // Note this IP for client requests

  // --- Start the Server ---
  server.begin();
  
  // --- Initialize LoRa ---
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(865E6)) { // Use 865 MHz for India
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("✅ LoRa Receiver/Gateway ready.");
}

void loop() {
  // --- Check for LoRa packets ---
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    loraDataFromTransmitter = received; // store the latest data

    Serial.print("📡 Received LoRa packet: ");
    Serial.print(received);
    Serial.print(" | RSSI: ");
    Serial.print(LoRa.packetRssi());
    Serial.print(" dBm | SNR: ");
    Serial.println(LoRa.packetSnr());
  }

  // --- Handle WiFi Clients ---
  WiFiClient client = server.available();
  if (client) {
    Serial.println("🌐 Client connected.");

    // Parse data: expecting "MQ135,MQ7"
    int firstComma = loraDataFromTransmitter.indexOf(',');
    if (firstComma != -1) {
      String mq135_val = loraDataFromTransmitter.substring(0, firstComma);
      String mq7_val = loraDataFromTransmitter.substring(firstComma + 1);

      // Convert raw values to PPM (dummy conversion, replace with real calibration)
      float mq135_ppm = mq135_val.toInt();
      float mq7_ppm = mq7_val.toInt();

      // Send response
      client.printf("MQ-135,%.2f\n", mq135_ppm);
      client.printf("MQ-7,%.2f\n", mq7_ppm);
    } else {
      // If no valid LoRa data yet
      client.println("No valid LoRa data received yet.");
    }

    client.stop();
    Serial.println("🌐 Client disconnected.");
  }
}
