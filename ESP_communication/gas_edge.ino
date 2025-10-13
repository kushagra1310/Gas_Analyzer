#include <SPI.h>
#include <LoRa.h>

// --- LoRa Pin Definitions ---
// IMPORTANT: Check your specific ESP32 LoRa board's pinout!
#define LORA_NSS    5   // SPI Chip Select
#define LORA_RST    14  // Reset
#define LORA_DIO0   2   // IRQ (Interrupt Request)

// --- Sensor Pin Definitions ---
#define MQ135_PIN   34  // ADC pin for MQ-135
#define MQ136_PIN   35  // ADC pin for MQ-136 change to 136
#define MQ137_PIN   36  // ADC pin for MQ-137

int counter = 0;
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Transmitter");

  // --- Initialize LoRa ---
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(865E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // --- Read Sensor Values ---
  // We'll send the raw ADC values (0-4095 for ESP32)
  int mq135_value = analogRead(MQ135_PIN);
  int mq136_value = analogRead(MQ136_PIN);
  int mq137_value = analogRead(MQ137_PIN);

  // --- Create Data Packet ---
  // Format: "MQ135_VALUE,MQ136_VALUE,MQ137_VALUE,PACKET_COUNT"
  String dataPacket = String(mq135_value) + "," + String(mq136_value) + ","+String(mq137_value) + "," + String(counter);

  // --- Send LoRa Packet ---
  LoRa.beginPacket();
  LoRa.print(dataPacket);
  LoRa.endPacket();

  Serial.print("Sending packet: ");
  Serial.println(dataPacket);

  counter++;
  // get the prediction form ml model
  Serial.println("Waiting for prediction...");
  long startTime = millis();
  bool replyReceived = false;

  while (millis() - startTime < 5000) { // Wait for 5 seconds
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String reply = "";
      while (LoRa.available()) {
        reply += (char)LoRa.read();
      }
      Serial.print("Prediction received: ");
      Serial.println(reply);
      replyReceived = true;
      break; // Exit the while loop
    }
  }

  if (!replyReceived) {
    Serial.println("No reply received, timeout.");
  }

  delay(5000); // Send data every 0.5 seconds

}
