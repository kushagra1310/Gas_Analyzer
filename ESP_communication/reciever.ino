// ESP32 LoRa Receiver Code

#include <SPI.h>
#include <LoRa.h>

// Define the pins used by the LoRa module
#define SCK     5    // GPIO5  -- SCK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     14   // GPIO14 -- RESET
#define DIO0    26   // GPIO26 -- IRQ (Interrupt Request)

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver Test");

  // Setup LoRa transceiver module
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  // Replace 915E6 with your frequency band
  // Must match the sender's frequency
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initialized OK! Waiting for packets...");
  
  // Put the radio into continuous receive mode
  LoRa.receive();
}

void loop() {
  // Try to parse a packet
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    // Received a packet
    Serial.print("Received packet: '");

    // Read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // Print RSSI of this packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}
