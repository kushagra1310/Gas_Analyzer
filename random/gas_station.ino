#include <SPI.h>
#include <LoRa.h>

// --- LoRa Pin Definitions ---
#define LORA_NSS    5    // SPI Chip Select
#define LORA_RST    14   // Reset
#define LORA_DIO0   26   // IRQ (Interrupt Request)

int counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Transmitter (Random Data)");

  // --- Initialize LoRa ---
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Seed random number generator
  randomSeed(analogRead(0));
}

void loop() {
  // --- Generate Random Sensor Values ---
  float mq135_value = random(0, 10001) / 10000.0;  // Simulated value
  float mq136_value = random(0, 10001) / 10000.0;  // Simulated value
  float mq137_value = random(0, 10001) / 10000.0;  // Simulated value

  // --- Create Data Packet ---
  String dataPacket = String(mq135_value) + "," + 
                      String(mq136_value) + "," + 
                      String(mq137_value) + "," + 
                      String(counter);

  // --- Send LoRa Packet ---
  LoRa.beginPacket();
  LoRa.print(dataPacket);
  LoRa.endPacket();

  Serial.print("Sending packet: ");
  Serial.println(dataPacket);

  counter++;

  // --- Wait for Prediction from Receiver ---
  Serial.println("Waiting for prediction...");
  long startTime = millis();
  bool replyReceived = false;

  while (millis() - startTime < 5000) { // Wait up to 5 seconds
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String reply = "";
      while (LoRa.available()) {
        reply += (char)LoRa.read();
      }
      Serial.print("Prediction received: ");
      Serial.println(reply);
      replyReceived = true;
      break;
    }
  }

  if (!replyReceived) {
    Serial.println("No reply received, timeout.");
  }

  delay(5000); // Send data every 5 seconds
}
