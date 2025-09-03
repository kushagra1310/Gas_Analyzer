// MQ-7 CO Sensor with Arduino
// Connect: VCC → 5V, GND → GND, AO → A0 (analog input)

int mq7Pin = A0;  // Analog pin where MQ7 is connected
int sensorValue = 0;  
float voltage = 0;
float ppm = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Read analog value (0 - 1023 for Arduino Uno)
  sensorValue = analogRead(mq7Pin);

  // Convert to voltage (for Uno: 5V reference, 10-bit ADC)
  voltage = sensorValue * (5.0 / 1023.0);

  // Approximate CO concentration calculation
  // NOTE: Proper calibration with datasheet curve is required for accuracy
  ppm = (voltage - 0.1) * 2000.0;  

  if (ppm < 0) ppm = 0; // avoid negative readings

  Serial.print("Analog Value: ");
  Serial.print(sensorValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.print(" V | CO Concentration ~ ");
  Serial.print(ppm);
  Serial.println(" ppm");

  delay(1000); // 1 second delay
}
