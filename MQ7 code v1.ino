/*
 * ESP32 MQ-7 Carbon Monoxide Sensor PPM Reading Sketch
 *
 * This sketch is adapted for an ESP32 board. It reads the analog output
 * from an MQ-7 CO sensor, calibrates it, and converts the reading to PPM.
 *
 * --- IMPORTANT HARDWARE REQUIREMENT ---
 * The ESP32's analog pins operate at 3.3V, while the MQ-7 sensor outputs a 5V signal.
 * You MUST use a voltage divider to step down the sensor's output before
 * connecting it to an ESP32 analog pin to avoid damaging the board.
 *
 * Voltage Divider Circuit:
 * MQ-7 AOUT ---[ R1 (10kΩ) ]---+--- ESP32 Analog Pin (e.g., GPIO 34)
 * |
 * [ R2 (20kΩ) ]
 * |
 * GND
 *
 * Connections:
 * - MQ-7 AOUT pin -> Voltage Divider circuit above
 * - ESP32 Analog Pin (e.g., GPIO 34) -> Junction of R1 and R2
 * - MQ-7 VCC pin -> 5V pin (from ESP32 board or external supply)
 * - MQ-7 GND pin -> ESP32 GND pin
 */

// --- Constants and Pin Definitions ---
// Use an ADC1 pin (GPIOs 32-39 are good choices)
const int SENSOR_ANALOG_PIN = 34;

// --- Voltage Divider Resistor Values (in Ohms) ---
// These must match the resistors you use in your hardware setup.
const float R1_VALUE = 10000.0;
const float R2_VALUE = 20000.0;

// --- Calibration and Sensor Characteristics ---
// The value of the load resistor (RL) on the module in Ohms.
// This is typically 10kΩ (10000 Ohms) for most MQ-7 modules.
const float LOAD_RESISTOR_VALUE = 10000.0;

// R0 is the sensor's resistance in clean air. This value will be
// determined automatically during the calibration phase.
float R0 = 0;

// --- Main Sketch ---

void setup() {
  // Initialize serial communication for debugging and output.
  Serial.begin(9600);
  while (!Serial); // Wait for serial port to connect.

  Serial.println("ESP32 MQ-7 Sensor Sketch - CO in PPM");
  Serial.println("------------------------------------");
  Serial.println("Warming up the sensor...");

  // Allow the sensor to preheat for a moment before calibration.
  // A longer warmup period will result in more accurate readings.
  delay(20000); // 20-second warmup

  calibrateSensor();
}

void loop() {
  // Read the current CO concentration in PPM.
  float ppm = getPPM();

  // Print the result to the Serial Monitor.
  Serial.print("CO Concentration: ");
  Serial.print(ppm);
  Serial.println(" PPM");

  // Wait for a couple of seconds before the next reading.
  delay(2000);
}

// --- Helper Functions ---

/**
 * @brief Calibrates the sensor to find its resistance in clean air (R0).
 * This function should be run in an environment with fresh, clean air.
 */
void calibrateSensor() {
  Serial.println("Starting calibration...");
  Serial.println("Ensure the sensor is in clean air.");
  delay(2000); // Wait for user to read message

  float sensor_voltage_sum = 0;
  // Take multiple readings to get a stable average.
  for (int i = 0; i < 100; i++) {
    sensor_voltage_sum += readOriginalSensorVoltage();
    delay(50);
  }
  float avg_sensor_voltage = sensor_voltage_sum / 100.0;

  // Calculate the sensor's resistance (Rs) in clean air.
  float rs_air = (5.0 * LOAD_RESISTOR_VALUE / avg_sensor_voltage) - LOAD_RESISTOR_VALUE;

  // From the datasheet, the Rs/R0 ratio in clean air is approximately 9.8.
  // We can use this to calculate R0.
  // R0 = Rs_air / 9.8
  R0 = rs_air / 9.8;

  Serial.println("Calibration complete.");
  Serial.print("Calculated R0: ");
  Serial.println(R0);
  Serial.println("------------------------------------");
}

/**
 * @brief Reads the sensor and calculates the CO concentration in PPM.
 * @return The calculated CO concentration in PPM.
 */
float getPPM() {
  // Get the current resistance of the sensor (Rs).
  float rs = readSensorResistance();

  // Calculate the Rs/R0 ratio.
  float ratio = rs / R0;

  // The datasheet for the MQ-7 shows a log-log graph of (Rs/R0) vs PPM.
  // We can approximate the CO curve with a power function: PPM = A * (Rs/R0)^B
  // For CO, a common approximation is: PPM = 100 * (Rs/R0)^-1.5
  float ppm = 100 * pow(ratio, -1.5);

  return ppm;
}

/**
 * @brief Reads the voltage at the ESP32 pin and calculates the original sensor voltage.
 * @return The sensor's actual output voltage (before the voltage divider).
 */
float readOriginalSensorVoltage() {
  int sensorValue = analogRead(SENSOR_ANALOG_PIN);
  // ESP32 ADC is 12-bit (0-4095) and reference voltage is 3.3V.
  float v_out_divided = (sensorValue / 4095.0) * 3.3;

  // Reverse the voltage divider formula to find the original sensor voltage.
  // V_sensor = V_out_divided * (R1 + R2) / R2
  return v_out_divided * (R1_VALUE + R2_VALUE) / R2_VALUE;
}

/**
 * @brief Calculates the sensor's current resistance (Rs).
 * @return The calculated resistance in Ohms.
 */
float readSensorResistance() {
  float sensor_volt = readOriginalSensorVoltage();
  // Use the voltage divider formula to calculate Rs from the original sensor voltage.
  // Rs = (Vc * RL / Vout) - RL
  // Vc = 5V
  float rs_gas = (5.0 * LOAD_RESISTOR_VALUE / sensor_volt) - LOAD_RESISTOR_VALUE;
  return rs_gas;
}
