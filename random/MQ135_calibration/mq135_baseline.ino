int mq135Pin = 34;             // ADC pin connected to MQ-135 AO via voltage divider
const int totalSamples = 10000;
const int sampleDelay = 100;   // ms between samples

// Circuit parameters
const float RL = 10.0;         // Load resistor in kΩ (standard MQ-135)
const float R1 = 20.0;         // Voltage divider resistor 1 (kΩ)
const float R2 = 20.0;         // Voltage divider resistor 2 (kΩ)
const float supplyVoltage = 5.0;

float rsValues[totalSamples];
unsigned long startTime;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  delay(2000);
  Serial.println("SampleNumber,ADCValue,VoltageOut,VoltageAO,Resistance_kOhm");
  startTime = millis();
}

void loop() {
  for (int i = 0; i < totalSamples; i++) {
    int adcVal = analogRead(mq135Pin);
    float vOut = (adcVal / 4095.0) * 3.3;  // Voltage at ADC pin after divider
    float vAO = vOut * ((R1 + R2) / R2);   // Recalculate sensor AO voltage
    float Rs = RL * ((supplyVoltage / vAO) - 1.0);  // Sensor resistance calculation

    rsValues[i] = Rs;

    Serial.print(i + 1); Serial.print(",");
    Serial.print(adcVal); Serial.print(",");
    Serial.print(vOut, 4); Serial.print(",");
    Serial.print(vAO, 4); Serial.print(",");
    Serial.println(Rs, 4);

    delay(sampleDelay);
  }

  // Calculate average baseline resistance R0
  float sum = 0;
  for (int i = 0; i < totalSamples; i++) {
    sum += rsValues[i];
  }
  float R0 = sum / totalSamples;

  Serial.print("Average R0 after ");
  Serial.print(totalSamples);
  Serial.print(" samples: ");
  Serial.print(R0, 4);
  Serial.println(" kOhm");

  unsigned long elapsed = millis() - startTime;
  Serial.print("Total time taken: ");
  Serial.print(elapsed / 1000.0, 2);
  Serial.println(" seconds");

  while (1); // Stop here
}
