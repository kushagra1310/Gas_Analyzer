#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

// --- Wi-Fi Credentials ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
WiFiServer server(8080); // Create a server on port 8080

// --- LoRa Pin Definitions ---
// IMPORTANT: Must match the transmitter's pins for your board model
#define LORA_NSS    5
#define LORA_RST    14
#define LORA_DIO0   2

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
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // IMPORTANT: Note this IP for your Python script

  // --- Start the Server ---
  server.begin();
  
  // --- Initialize LoRa ---
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) { // Must use the same frequency as the transmitter
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Receiver/Gateway is ready.");
}

void loop() {
  // ... your LoRa receiving logic to update loraDataFromTransmitter ...
  // Example: loraDataFromTransmitter = "1234,567" (raw MQ135, MQ7 values)
  
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected.");
    
    // --- PARSE AND SEND DATA ---
    // Assuming LoRa data is "MQ135_VALUE,MQ7_VALUE"
    int firstComma = loraDataFromTransmitter.indexOf(',');
    if (firstComma != -1) {
      String mq135_val = loraDataFromTransmitter.substring(0, firstComma);
      String mq7_val = loraDataFromTransmitter.substring(firstComma + 1);

      // Convert raw ADC values to PPM (this is a placeholder, you'll need a real formula)
      float mq135_ppm = map(mq135_val.toInt(), 0, 4095, 50, 300);
      float mq7_ppm = map(mq7_val.toInt(), 0, 4095, 50, 300);

      // Send each sensor reading as a separate, newline-terminated string
      client.printf("MQ-135,%.2f\n", mq135_ppm);
      client.printf("MQ-7,%.2f\n", mq7_ppm);

      Serial.println("Sent data to client.");
    }

    client.stop();
    Serial.println("Client disconnected.");
  }
}