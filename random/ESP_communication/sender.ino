// ESP32 LoRa Sender Code

#include <SPI.h>
#include <LoRa.h>

// Define the pins used by the LoRa module
#define SCK     5    // GPIO5  -- SCK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     14   // GPIO14 -- RESET
#define DIO0    26   // GPIO26 -- IRQ (Interrupt Request)

int counter = 0;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Sender Test");

  // Setup LoRa transceiver module
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  // Replace 915E6 with your frequency band
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initialized OK!");
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // Begin a packet
  LoRa.beginPacket();
  // Add payload
  LoRa.print("Hello from Sender! Packet #");
  LoRa.print(counter);
  // Finish packet and send it
  LoRa.endPacket();

  counter++;

  delay(5000); // Wait 5 seconds between transmissions
}
