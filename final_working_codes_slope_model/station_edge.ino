#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Nothing Phone (2)";
const char* password = "12345678";
WiFiServer server(8080);

// REPLACE with your PC's actual IP
const char* knnServerUrl = "http://10.77.54.12:5000/predict";

// LoRa Pins
#define LORA_NSS    5
#define LORA_RST    14
#define LORA_DIO0   26

String loraDataFromTransmitter = "No data yet";

// Function to send data (including slopes) to Flask
String getModelPrediction(float mq3, float mq136, float mq137, float s3, float s136, float s137) {
  HTTPClient http;
  String prediction = "Error";

  // Construct URL with slopes
  String requestUrl = String(knnServerUrl) + 
                      "?mq3=" + String(mq3, 3) + 
                      "&mq136=" + String(mq136, 3) + 
                      "&mq137=" + String(mq137, 3) +
                      "&s_mq3=" + String(s3, 3) +
                      "&s_mq136=" + String(s136, 3) +
                      "&s_mq137=" + String(s137, 3);

  Serial.println(requestUrl);

  if (http.begin(requestUrl)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      prediction = http.getString();
    } else {
      Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  return prediction;
}

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());

  server.begin();

  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed!");
    while (1);
  }
}

void loop() {
  // --- Receive LoRa ---
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) received += (char)LoRa.read();
    loraDataFromTransmitter = received;
    Serial.println("RX: " + received);
  }

  // --- Handle WiFi Clients ---
  WiFiClient client = server.available();
  if (client) {
    // Expecting: MQ3, MQ136, MQ137, Slope3, Slope136, Slope137, Counter
    // We need to find 6 commas to split 7 items
    
    int c1 = loraDataFromTransmitter.indexOf(',');
    int c2 = loraDataFromTransmitter.indexOf(',', c1 + 1);
    int c3 = loraDataFromTransmitter.indexOf(',', c2 + 1);
    int c4 = loraDataFromTransmitter.indexOf(',', c3 + 1);
    int c5 = loraDataFromTransmitter.indexOf(',', c4 + 1);
    int c6 = loraDataFromTransmitter.indexOf(',', c5 + 1);

    if (c1 != -1 && c5 != -1) {
      // Extract Values
      float mq3 = loraDataFromTransmitter.substring(0, c1).toFloat();
      float mq136 = loraDataFromTransmitter.substring(c1 + 1, c2).toFloat();
      float mq137 = loraDataFromTransmitter.substring(c2 + 1, c3).toFloat();
      
      // Extract Slopes
      float s_mq3 = loraDataFromTransmitter.substring(c3 + 1, c4).toFloat();
      float s_mq136 = loraDataFromTransmitter.substring(c4 + 1, c5).toFloat();
      float s_mq137 = loraDataFromTransmitter.substring(c5 + 1, c6).toFloat();

      // Get Prediction
      String prediction = getModelPrediction(mq3, mq136, mq137, s_mq3, s_mq136, s_mq137);
      
      // Send Reply to LoRa Transmitter
      LoRa.beginPacket();
      LoRa.print(prediction);
      LoRa.endPacket();

      // Send Data to GUI (Client)
      // We send the averages to the GUI so the graph looks smooth
      client.printf("MQ-3,%.2f\n", mq3);
      client.printf("MQ-136,%.2f\n", mq136);
      client.printf("MQ-137,%.2f\n", mq137);
      client.printf("Prediction,%s\n", prediction.c_str());
    } else {
      client.println("Waiting for data...");
    }
    client.stop();
  }
}