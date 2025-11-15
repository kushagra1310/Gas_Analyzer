#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8g2lib.h>
// #include <Mhz19.h> 

// ---------- Sensor Pins ----------
#define MQ136_PIN 27
#define MQ3_PIN 33
#define MQ137_PIN 12
// #define MHZ19_RX 32  // ESP32 RX (connects to MH-Z19 TX)
// #define MHZ19_TX 13  // ESP32 TX (connects to MH-Z19 RX)

// --- LoRa Pin Definitions ---
#define LORA_NSS    5    // SPI Chip Select
#define LORA_RST    14   // Reset
#define LORA_DIO0   26   // IRQ (Interrupt Request)

// --- Buzzer Pin Definition ---
#define BUZZER_PIN  25   // Buzzer connected to GPIO 25

// ---------- Constants ----------
const int CALIBRATION_SAMPLES = 100;   // Number of samples for Ro calibration
const int SAMPLE_INTERVAL = 1000;      // 1 second between readings
const float RL = 10.0;                 // Load resistor in kΩ
const float ADC_MAX = 4095.0;          // 12-bit ADC (0–4095)
const float VREF = 3.3;                // ADC reference voltage

// --- Objects ---
// OLED Display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// MH-Z19 Setup (Fixed based on your working code)
HardwareSerial mySerial(2); // Use UART2
// Mhz19 myMhz19;

// ---------- Global Variables ----------
float Ro3 = 0.0, Ro136 = 0.0, Ro137 = 0.0;
bool calibrated = false;
int counter = 0;
String prev_pred = "";
int last_co2_ppm = 400; // Default fallback value

// ---------- Helper Functions ----------
float readSensorVoltage(int pin) {
  int adcValue = analogRead(pin);
  return (adcValue * VREF / ADC_MAX);
}

float calculateRs(float vOut) {
  if (vOut <= 0.0) return -1.0;
  return RL * (VREF - vOut) / vOut; // Rs = RL * (Vc - Vout) / Vout
}

void displayPrediction(String pred) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 30, "Prediction:");
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(20, 55, pred.c_str());
  u8g2.sendBuffer();
}

void calibrateSensors() {
  Serial.println("======================================================");
  Serial.println("     MQ-3 / MQ-136 / MQ-137 CALIBRATION STARTED     ");
  Serial.println("======================================================");

  float sum3 = 0, sum136 = 0, sum137 = 0;

  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    float v3 = readSensorVoltage(MQ3_PIN);
    float v136 = readSensorVoltage(MQ136_PIN);
    float v137 = readSensorVoltage(MQ137_PIN);

    float Rs3 = calculateRs(v3);
    float Rs136 = calculateRs(v136);
    float Rs137 = calculateRs(v137);

    sum3 += Rs3;
    sum136 += Rs136;
    sum137 += Rs137;

    Serial.printf("[Sample %3d/%d] Rs3=%.3f | Rs136=%.3f | Rs137=%.3f\n",
                  i + 1, CALIBRATION_SAMPLES, Rs3, Rs136, Rs137);
    delay(200); // Small delay between samples
  }

  Ro3 = sum3 / CALIBRATION_SAMPLES;
  Ro136 = sum136 / CALIBRATION_SAMPLES;
  Ro137 = sum137 / CALIBRATION_SAMPLES;
  calibrated = true;

  Serial.println("\n✅ Calibration Complete!");
  Serial.printf("  Ro3 (MQ-3)     = %.3f kΩ\n", Ro3);
  Serial.printf("  Ro136 (MQ-136) = %.3f kΩ\n", Ro136);
  Serial.printf("  Ro137 (MQ-137) = %.3f kΩ\n", Ro137);
  Serial.println("======================================================");
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Transmitter (Calibrated Sensors)");

  // --- Initialize Buzzer Pin ---
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off

  // --- Initialize OLED Display ---
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(10, 30, "Gas Analyzer");
  u8g2.drawStr(20, 50, "Starting...");
  u8g2.sendBuffer();

  // --- Initialize LoRa ---
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    u8g2.clearBuffer();
    u8g2.drawStr(10, 30, "LoRa Failed!");
    u8g2.sendBuffer();
    while (1);
  }

  // // --- Initialize MH-Z19 (Fixed Logic) ---
  // mySerial.begin(9600, SERIAL_8N1, MHZ19_RX, MHZ19_TX);
  // myMhz19.begin(&mySerial);
  // myMhz19.disableAutoBaseCalibration();

  // Serial.println("MH-Z19 warming up (3 min recommended)...");
  // u8g2.clearBuffer();
  // u8g2.drawStr(20, 30, "Warming up...");
  // u8g2.sendBuffer();
   
  // Display ready message
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(25, 30, "Calibrating...");
  u8g2.sendBuffer();

  calibrateSensors();

  u8g2.clearBuffer();
  u8g2.drawStr(25, 30, "System Ready");
  u8g2.sendBuffer();
  delay(2000);
}

void loop() {
  if (!calibrated) return;

  // --- Read Analog Sensor Values ---
  float v3 = readSensorVoltage(MQ3_PIN);
  float v136 = readSensorVoltage(MQ136_PIN);
  float v137 = readSensorVoltage(MQ137_PIN);

  float Rs3 = calculateRs(v3);
  float Rs136 = calculateRs(v136);
  float Rs137 = calculateRs(v137);

  float mq3_value = Rs3 / Ro3;
  float mq136_value = Rs136 / Ro136;
  float mq137_value = Rs137 / Ro137;

  // // --- Read CO2 Sensor---
  // int mhz19_value = -1;
  // if (myMhz19.isReady()) {
  //   mhz19_value = myMhz19.getCarbonDioxide();
  //   if (mhz19_value > 0) last_co2_ppm = mhz19_value;
  //   Serial.printf("CO₂ (ppm): %d\n", mhz19_value);
  // } else {
  //   Serial.println("MH-Z19 not ready, using last valid CO₂ value.");
  //   mhz19_value = last_co2_ppm;
  // }


  // --- Create Data Packet ---
  String dataPacket = String(mq3_value) + "," + 
                      String(mq136_value) + "," + 
                      String(mq137_value) + "," + 
                      // String(mhz19_value) + "," + 
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
   
  // Display waiting status on OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(5, 15, "Waiting...");
  u8g2.sendBuffer();
   
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
      
      // Display prediction on OLED
      displayPrediction(reply);
      
      // Check if gas has changed and trigger buzzer
      if(prev_pred != "" && prev_pred != reply){
        Serial.println("Gas has changed.");
        // Turn on buzzer for 0.5 seconds
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
      }
      prev_pred = reply;
      break;
    }
  }

  if (!replyReceived) {
    Serial.println("No reply received, timeout.");
    
    // Display timeout on OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(15, 30, "No Response");
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(30, 50, "Timeout");
    u8g2.sendBuffer();
  }

  delay(5000); // Send data every 5 seconds
}
