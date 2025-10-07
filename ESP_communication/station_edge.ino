#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h> // --- NEW: Library for making HTTP requests ---

// --- Wi-Fi Credentials ---
const char* ssid = "Nothing Phone (2)";
const char* password = "12345678";
WiFiServer server(8080); // Create a server on port 8080

// --- NEW: URL of your Python KNN Model Server ---
// IMPORTANT: Replace <ip> with the actual local IP of the PC running the Python script
const char* knnServerUrl = "http://127.0.0.1:5000/predict";

// --- LoRa Pin Definitions ---
#define LORA_NSS    5
#define LORA_RST    14
#define LORA_DIO0   2

// Store last received LoRa data
String loraDataFromTransmitter = "No data yet";

// --- NEW: Function to get prediction from KNN Model Server ---
String getModelPrediction(float mq135, float mq7) {
  HTTPClient http;
  String prediction = "Prediction Error"; // Default error message

  // Create the full URL with query parameters
  String requestUrl = String(knnServerUrl) + "?mq135=" + String(mq135, 2) + "&mq7=" + String(mq7, 2);
  
  Serial.print("Making request to: ");
  Serial.println(requestUrl);

  if (http.begin(requestUrl)) {
    int httpCode = http.GET(); // Send the GET request
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        prediction = http.getString(); // Get the response payload (the prediction)
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTP] Unable to connect\n");
  }

  return prediction;
}


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

    Serial.print("Received LoRa packet: ");
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
      
      float mq135_ppm = mq135_val.toFloat();
      float mq7_ppm = mq7_val.toFloat();
      
      // Send response
      client.printf("MQ-135,%.2f\n", mq135_ppm);
      client.printf("MQ-7,%.2f\n", mq7_ppm);
    } else {
      // If no valid LoRa data yet
      client.println("No valid LoRa data received yet.");
    }

    client.stop();
    Serial.println("🌐 Client disconnected.");

    // --- NEW: Get prediction from the model ---
    String prediction = getModelPrediction(mq135_ppm, mq7_ppm);
    Serial.print("Model Prediction: ");
    Serial.println(prediction);
    Serial.println("Sending prediction back via LoRa...");
    LoRa.beginPacket();
    LoRa.print(prediction);
    LoRa.endPacket();
    Serial.println("✅ LoRa packet sent.");
  }
}
