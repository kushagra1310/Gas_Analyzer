/*
 * MQ-7 Carbon Monoxide Sensor PPM Reading Sketch
 *
 * This sketch reads the analog output from an MQ-7 CO sensor,
 * calibrates it, and converts the reading to Parts Per Million (PPM).
 *
 * Connections:
 * - MQ-7 AOUT pin -> Arduino A0 pin
 * - MQ-7 VCC pin -> Arduino 5V pin
 * - MQ-7 GND pin -> Arduino GND pin
 *
 * How it works:
 * 1. Calibration: The sensor first needs to be calibrated to find its
 * resistance in clean air (R0). This sketch does a quick calibration
 * in the setup() function. For best results, let the sensor warm up
 * for several minutes before running the calibration. The datasheet
 * recommends a 48-hour preheating period for maximum stability.
 *
 * 2. PPM Conversion: The sensor's resistance (Rs) changes in the presence
 * of CO. The ratio of Rs to R0 (Rs/R0) is used to calculate the PPM
 * concentration based on a formula derived from the sensor's datasheet graph.
 *
 * IMPORTANT: This sensor is for educational and hobbyist purposes.
 * For any application where CO poses a health risk, use a certified and
 * professionally calibrated CO detector.
 */

// --- Constants and Pin Definitions ---
const int SENSOR_ANALOG_PIN = A0; // Analog pin connected to the MQ-7's AOUT

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
  while (!Serial); // Wait for serial port to connect. Needed for native USB port only

  Serial.println("MQ-7 Sensor Sketch - CO in PPM");
  Serial.println("--------------------------------");
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
    sensor_voltage_sum += readSensorVoltage();
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
  Serial.println("--------------------------------");
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
  // From the graph, we can find two points and solve for A and B.
  // For CO, a common approximation is: PPM = 100 * (Rs/R0)^-1.5
  // Note: These values might need tweaking for your specific sensor.
  float ppm = 100 * pow(ratio, -1.5);

  return ppm;
}

/**
 * @brief Reads the raw analog value and converts it to voltage.
 * @return The sensor's output voltage.
 */
float readSensorVoltage() {
  int sensorValue = analogRead(SENSOR_ANALOG_PIN);
  // Convert the analog value (0-1023) to a voltage (0-5V).
  return (sensorValue / 1023.0) * 5.0;
}

/**
 * @brief Calculates the sensor's current resistance (Rs).
 * @return The calculated resistance in Ohms.
 */
float readSensorResistance() {
  float sensor_volt = readSensorVoltage();
  // Use the voltage divider formula to calculate Rs.
  // Rs = (Vc * RL / Vout) - RL
  // Vc = 5V
  float rs_gas = (5.0 * LOAD_RESISTOR_VALUE / sensor_volt) - LOAD_RESISTOR_VALUE;
  return rs_gas;
}
