#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Nothing Phone (2)";
const char* password = "12345678";
WiFiServer server(8080); // Create a server on port 8080

// IMPORTANT: Replace <ip> with the actual local IP of the PC running the Python script
const char* ServerUrl = "http://10.77.54.12:5000/predict";

// --- LoRa Pin Definitions ---
#define LORA_NSS    5
#define LORA_RST    14
#define LORA_DIO0   26

// Store last received LoRa data
String loraDataFromTransmitter = "No data yet";

// String getModelPrediction(float mq3, float mq136, float mq137, float mhz19) {
//   HTTPClient http;
//   String prediction = "Prediction Error"; // Default error message

//   // Create the full URL with query parameters
//   String requestUrl = String(ServerUrl) + "?mq3=" + String(mq3, 2) + "&mq136=" + String(mq136, 2) + "&mq137=" + String(mq137, 2) + "&mhz19=" + String(mhz19, 2);
  
//   Serial.print("Making request to: ");
//   Serial.println(requestUrl);

//   if (http.begin(requestUrl)) {
//     int httpCode = http.GET(); // Send the GET request
//     if (httpCode > 0) {
//       if (httpCode == HTTP_CODE_OK) {
//         prediction = http.getString(); // Get the response payload (the prediction)
//       } else {
//         Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
//       }
//     } else {
//       Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
//     }
//     http.end();
//   } else {
//     Serial.printf("[HTTP] Unable to connect\n");
//   }

//   return prediction;
// }

String getModelPrediction(float mq3, float mq136, float mq137) {
  HTTPClient http;
  String prediction = "Prediction Error"; // Default error message

  // Create the full URL with query parameters
  String requestUrl = String(ServerUrl) + "?mq3=" + String(mq3, 2) + "&mq136=" + String(mq136, 2) + "&mq137=" + String(mq137, 2);
  
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
  if (!LoRa.begin(433E6)) { // Use 865 MHz for India
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
    
    // Parse data: expecting "MQ3,MQ136,MQ137,MHZ19,counter"
    int firstComma = loraDataFromTransmitter.indexOf(',');
    int secondComma = loraDataFromTransmitter.indexOf(',', firstComma + 1);
    int thirdComma = loraDataFromTransmitter.indexOf(',', secondComma + 1);
    // int fourthComma = loraDataFromTransmitter.indexOf(',', thirdComma + 1);
    if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
      String mq3_val = loraDataFromTransmitter.substring(0, firstComma);
      String mq136_val = loraDataFromTransmitter.substring(firstComma + 1, secondComma);
      String mq137_val = loraDataFromTransmitter.substring(secondComma + 1, thirdComma);
      // String mhz19_val = loraDataFromTransmitter.substring(thirdComma + 1, fourthComma);
      
      float mq3_res = mq3_val.toFloat();
      float mq136_res = mq136_val.toFloat();
      float mq137_res = mq137_val.toFloat();
      // float mhz19_res = mhz19_val.toFloat();
      
      // --- Get prediction from the model ---
      String prediction = getModelPrediction(mq3_res, mq136_res, mq137_res);
      // String prediction = getModelPrediction(mq3_res, mq136_res, mq137_res, mhz19_res);
      Serial.print("Model Prediction: ");
      Serial.println(prediction);
      Serial.println("Sending prediction back via LoRa...");
      LoRa.beginPacket();
      LoRa.print(prediction);
      LoRa.endPacket();
      Serial.println("✅ LoRa packet sent.");
      
      // Send response
      client.printf("MQ-3,%.2f\n", mq3_res);
      client.printf("MQ-136,%.2f\n", mq136_res);
      client.printf("MQ-137,%.2f\n", mq137_res);
      // client.printf("MHZ19,%.2f\n", mhz19_res);
      client.printf("Prediction,%s\n", prediction);
    } else {
      // If no valid LoRa data yet
      client.println("No valid LoRa data received yet.");
    }

    client.stop();
    Serial.println("🌐 Client disconnected.");

  }
}