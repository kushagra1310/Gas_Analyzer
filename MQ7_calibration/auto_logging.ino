/**
 * @file esp32_mq7_calibration.ino
 * @brief Calibrates an MQ-7 CO sensor and logs data for curve fitting.
 * @author Amy for Luffy
 *
 * Hardware Setup:
 * - ESP32
 * - MQ-7 Sensor
 * - VCC of sensor -> 5V
 * - GND of sensor -> GND
 * - AOUT of sensor -> One end of the voltage divider
 * - Voltage Divider:
 * - AOUT --- R1 (20k) --- ESP32 PIN 34 --- R2 (20k) --- GND
 * - A separate Load Resistor (RL) is also part of the sensor circuit, connecting AOUT to GND.
 *
 * Calibration Process:
 * 1. Pre-heat: Ensure the sensor is powered on for at least 30 minutes before starting.
 * 2. Find Ro: The code first measures the sensor's resistance in clean air for 5 minutes
 * and calculates the average, which becomes Ro.
 * 3. Log Data: After Ro is fixed, the program prompts you to enter known PPM values
 * via the Serial Monitor. For each entry, it calculates the current Rs/Ro ratio
 * and prints a CSV line.
 * 4. End: Type "end" to finish the process. The final Ro value will be printed.
 * 5. Copy Data: Copy the entire CSV output from the Serial Monitor into a new file
 * and save it as 'calibration_data.csv'.
 */

// --- Pin and Circuit Configuration ---
const int MQ7_ADC_PIN = 34; // ADC pin connected to the middle of the voltage divider

// !!! IMPORTANT: CHANGE THIS TO YOUR ACTUAL LOAD RESISTOR VALUE IN kOhms !!!
const float R_LOAD = 10.0; // The load resistor (RL) value in kOhms.

// Voltage divider resistors (as you specified)
const float R1 = 20.0; // Resistor from Sensor AOUT to ADC PIN (kOhms)
const float R2 = 20.0; // Resistor from ADC PIN to GND (kOhms)

// --- Voltage & ADC Configuration ---
const float SUPPLY_VOLTAGE = 5.0;  // Voltage powering the MQ-7 sensor circuit
const float ADC_VREF = 3.3;        // ESP32 ADC reference voltage
const int ADC_RESOLUTION = 4095; // 12-bit ADC (0-4095)

// --- Calibration Configuration ---
const unsigned long RO_CALIBRATION_DURATION_MS = 5 * 60 * 1000; // 5 minutes
const unsigned long RO_SAMPLING_INTERVAL_MS = 5000;             // Sample every 5 seconds

// --- Global Variables ---
float Ro = -1.0;          // Will hold the calculated Ro value. Initialized to an invalid value.
int serial_number = 1;    // Counter for the CSV file

// State machine to manage the process
enum State {
  WAIT_FOR_START,
  CALIBRATE_RO,
  LOG_DATA,
  END_PROCESS
};
State currentState = WAIT_FOR_START;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor to open

  pinMode(MQ7_ADC_PIN, INPUT);

  Serial.println("MQ-7 Sensor Calibration Sketch");
  Serial.println("--------------------------------");
  Serial.println("IMPORTANT: Ensure the sensor has been pre-heating in clean air for at least 30 minutes.");
  Serial.println("Press ENTER to begin the 5-minute Ro calibration in clean air.");
}

void loop() {
  switch (currentState) {
    case WAIT_FOR_START:
      if (Serial.available() > 0) {
        // Clear the buffer
        while(Serial.read() != -1);
        currentState = CALIBRATE_RO;
      }
      break;

    case CALIBRATE_RO:
      calculateRoInCleanAir();
      Serial.println("\nRo calibration complete.");
      Serial.print("Calculated Ro = ");
      Serial.println(Ro, 4);
      Serial.println("\n--- Data Logging Phase ---");
      Serial.println("Place the sensor in the environment with your industrial sensor.");
      Serial.println("Enter a known PPM value (e.g., '150.5') and press Enter to log the Rs/Ro ratio.");
      Serial.println("Type 'end' and press Enter to finish.");
      Serial.println("\n--- CSV Data ---");
      Serial.println("sno,ppm,rs/ro"); // Print CSV header
      currentState = LOG_DATA;
      break;

    case LOG_DATA:
      handleDataLogging();
      break;

    case END_PROCESS:
      Serial.println("\n--- Process Finished ---");
      Serial.print("# Final Ro value used for calculations: "); // Using '#' makes it a comment in CSV
      Serial.println(Ro, 4);
      Serial.println("# Copy the CSV data above (starting with the header) into a .csv file.");
      // Halt execution
      while (true) {
        delay(1000);
      }
      break;
  }
}

/**
 * @brief Reads the sensor for 5 minutes and calculates the average resistance in clean air (Ro).
 */
void calculateRoInCleanAir() {
  Serial.println("\nStarting Ro calibration. Please wait for 5 minutes...");
  float total_rs = 0.0;
  int readings_count = 0;
  unsigned long startTime = millis();
  unsigned long nextSampleTime = startTime;

  while (millis() - startTime < RO_CALIBRATION_DURATION_MS) {
    if (millis() >= nextSampleTime) {
      float rs = calculateRs();
      if (rs >= 0) {
        total_rs += rs;
        readings_count++;
      }
      Serial.print(".");
      nextSampleTime += RO_SAMPLING_INTERVAL_MS;
    }
  }

  if (readings_count > 0) {
    Ro = total_rs / readings_count;
  } else {
    Ro = 0; // Handle case where no readings were taken
  }
}

/**
 * @brief Handles serial input for logging PPM values and corresponding Rs/Ro ratios.
 */
void handleDataLogging() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("end")) {
      currentState = END_PROCESS;
      return;
    }

    float ppm_value = input.toFloat();
    if (ppm_value > 0) {
      float current_rs = calculateRs();
      if (current_rs >= 0 && Ro > 0) {
        float ratio = current_rs / Ro;

        // Print the data in CSV format
        Serial.print(serial_number);
        Serial.print(",");
        Serial.print(ppm_value, 2);
        Serial.print(",");
        Serial.println(ratio, 4);

        serial_number++;
      } else {
         Serial.println("Error: Could not calculate valid Rs/Ro. Check sensor connections and Ro value.");
      }
    } else {
      Serial.println("Invalid input. Please enter a positive number for PPM or 'end'.");
    }
  }
}

/**
 * @brief Reads the sensor's ADC value and calculates its current resistance (Rs).
 * @return The calculated sensor resistance in kOhms, or -1.0 on error.
 */
float calculateRs() {
  int adcVal = analogRead(MQ7_ADC_PIN);

  // Convert ADC value to voltage at the divider's midpoint
  float vOut = (adcVal / (float)ADC_RESOLUTION) * ADC_VREF;

  // Recalculate the sensor's original output voltage (V_AO) before the divider
  // V_AO = V_out * (R1 + R2) / R2
  float vAO = vOut * ((R1 + R2) / R2);

  // Prevent division by zero if vAO is 0
  if (vAO == 0) {
    return -1.0;
  }

  // Calculate sensor resistance (Rs) using the formula from the datasheet
  // Rs = RL * (Vcc - V_AO) / V_AO
  float Rs = R_LOAD * ((SUPPLY_VOLTAGE / vAO) - 1.0);

  return Rs;
}
