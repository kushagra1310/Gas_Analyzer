int mq7Pin = 34;            // ADC pin
const int totalSamples = 1000;
const int sampleDelay = 300; // ms between samples

// Voltage divider params
const float R1 = 20.0;      // kΩ
const float R2 = 20.0;      // kΩ
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

  for(int i = 0; i < totalSamples; i++) {
    int adcVal = analogRead(mq7Pin);
    float vOut = (adcVal / 4095.0) * 3.3;
    float vAO = vOut * 2; // Divider ratio 0.5
    float Rs = ((supplyVoltage * (R1 + R2)) / vAO) - (R1 + R2);
    rsValues[i] = Rs;

    // CSV format: sample number, raw ADC, voltage at ADC, AO voltage, resistance
    Serial.print(i+1); Serial.print(",");
    Serial.print(adcVal); Serial.print(",");
    Serial.print(vOut, 4); Serial.print(",");
    Serial.print(vAO, 4); Serial.print(",");
    Serial.println(Rs, 4);

    delay(sampleDelay);
  }

  // Calculate average R0
  float sum = 0;
  for(int i = 0; i < totalSamples; i++) {
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

  while(1); // Stop after calibration is done
}
