/*
 * ESP32 MQ-7 Sensor - Simplified Sketch for Constant 5V Heating
 *
 * --- IMPORTANT ---
 * This sketch is a compromise for when you DON'T have a MOSFET to control the heater.
 * It will be less accurate than the proper dual-heat method.
 *
 * --- CRITICAL USAGE NOTE ---
 * A VERY LONG WARM-UP (1-2 hours minimum) is required for the sensor to stabilize.
 * Calibration should only be performed after this long warm-up period.
 *
 * --- CONNECTIONS ---
 * - MQ-7 VCC & H pin -> 5V
 * - MQ-7 GND & other H pin -> GND
 * - MQ-7 AOUT -> Voltage Divider -> SENSOR_ANALOG_PIN (e.g., 34)
 */

// --- Pin Definitions ---
const int SENSOR_ANALOG_PIN = 34; // Use an ADC1 pin (GPIOs 32-39)

// --- Voltage Divider Resistor Values (in Ohms) ---
const float R1_VALUE = 10000.0;
const float R2_VALUE = 20000.0;

// --- Sensor Characteristics ---
const float LOAD_RESISTOR_VALUE = 10000.0; // Value of the load resistor on the module
float R0 = 0.0; // Will be determined during calibration.

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("ESP32 MQ-7 Sensor Sketch (Simplified, Constant 5V Heat)");
  Serial.println("-------------------------------------------------------");
  Serial.println("Warming up the sensor...");
  Serial.println("NOTE: For accurate calibration, a warm-up of 1-2 hours is recommended.");
  Serial.println("This sketch will proceed after 30 seconds for initial testing.");
  
  // A short delay for quick testing, but remember the long warm-up rule!
  delay(30000); 

  calibrateSensor();
}

void loop() {
  float ppm = getPPM();

  // Print all the intermediate values for easy debugging
  Serial.println("--- New Reading ---");
  float rs_current = readSensorResistance();
  float ratio = rs_current / R0;
  
  Serial.print("Current Sensor Resistance (Rs): ");
  Serial.println(rs_current);
  Serial.print("Current Ratio (Rs/R0): ");
  Serial.println(ratio);
  Serial.print("CO Concentration: ");
  Serial.print(ppm);
  Serial.println(" PPM");

  // Wait for 5 seconds before the next reading.
  delay(5000);
}

/**
 * @brief Calibrates the sensor to find R0 in clean air.
 * THIS IS THE MOST IMPORTANT FUNCTION TO GET RIGHT.
 */
void calibrateSensor() {
  Serial.println("\nStarting calibration...");
  Serial.println("Ensure the sensor is in clean air AND has been warmed up for at least 1-2 hours.");
  delay(2000); // Wait for user to read message

  float rs_air_sum = 0;
  // Take multiple readings to get a stable average.
  Serial.println("Taking 50 readings for calibration average...");
  for (int i = 0; i < 50; i++) {
    rs_air_sum += readSensorResistance();
    delay(100);
  }
  float rs_air = rs_air_sum / 50.0;
  Serial.print("Average sensor resistance in clean air (Rs_air): ");
  Serial.println(rs_air);

  // --- CRITICAL CHANGE ---
  // When heating with constant 5V, the "clean air" ratio (Rs/R0) is NOT 9.8.
  // It is much lower, typically around 2.6 to 3.0. We will use 2.8 as a starting point.
  // You may need to adjust this value slightly for your specific sensor.
  const float CLEAN_AIR_RATIO = 2.8; 
  R0 = rs_air / CLEAN_AIR_RATIO;

  Serial.println("Calibration complete.");
  Serial.print("Calculated R0: ");
  Serial.println(R0);
  Serial.println("-------------------------------------------------------");
}

/**
 * @brief Reads the sensor and calculates the CO concentration in PPM.
 * @return The calculated CO concentration in PPM.
 */
float getPPM() {
  float rs = readSensorResistance();
  if (rs <= 0) return 0; // Prevent errors
  
  float ratio = rs / R0;
  
  // The PPM curve formula remains the same. The accuracy now depends entirely on R0.
  // PPM = 100 * (Rs/R0)^-1.5
  float ppm = 100 * pow(ratio, -1.5);
  return ppm;
}

/**
 * @brief Reads the voltage at the ESP32 pin and calculates the original 5V sensor voltage.
 * @return The sensor's actual output voltage (before the voltage divider).
 */
float readOriginalSensorVoltage() {
  int sensorValue = analogRead(SENSOR_ANALOG_PIN);
  float v_out_divided = (sensorValue / 4095.0) * 3.3;

  // Reverse the voltage divider formula: V_sensor = V_out_divided * (R1 + R2) / R2
  return v_out_divided * (R1_VALUE + R2_VALUE) / R2_VALUE;
}

/**
 * @brief Calculates the sensor's current resistance (Rs).
 * @return The calculated resistance in Ohms.
 */
float readSensorResistance() {
  float sensor_volt = readOriginalSensorVoltage();
  if (sensor_volt <= 0) return 0; // Avoid division by zero
  
  // Rs = (Vc * RL / Vout) - RL  where Vc = 5V
  float rs_gas = (5.0 * LOAD_RESISTOR_VALUE / sensor_volt) - LOAD_RESISTOR_VALUE;
  return rs_gas < 0 ? 0 : rs_gas; // Ensure resistance is not negative
}
