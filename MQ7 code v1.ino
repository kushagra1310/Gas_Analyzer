/*
 * ESP32 MQ-7 Carbon Monoxide Sensor - Full Working Sketch
 *
 * This sketch correctly implements the dual-heat cycle required for the MQ-7 sensor.
 *
 * --- HARDWARE REQUIREMENTS ---
 * 1.  A voltage divider is REQUIRED on the AOUT pin (see original sketch).
 * 2.  A MOSFET or BJT transistor is REQUIRED to control the heater element.
 * The ESP32's GPIO pin cannot power the heater directly.
 *
 * --- CONNECTIONS ---
 * - MQ-7 AOUT -> Voltage Divider -> SENSOR_ANALOG_PIN (e.g., 34)
 * - MQ-7 VCC -> 5V
 * - MQ-7 GND -> GND
 * - MOSFET Gate -> HEATER_CONTROL_PIN (e.g., 23)
 * - MOSFET Drain -> One of the MQ-7 'H' pins
 * - MOSFET Source -> GND
 * - The other MQ-7 'H' pin -> 5V
 */

// --- Pin Definitions ---
const int SENSOR_ANALOG_PIN = 34;    // ADC1 pin for sensor reading
const int HEATER_CONTROL_PIN = 23;   // GPIO to control the MOSFET gate

// --- Voltage Divider Resistor Values (in Ohms) ---
const float R1_VALUE = 10000.0;
const float R2_VALUE = 20000.0;

// --- Sensor Characteristics ---
const float LOAD_RESISTOR_VALUE = 10000.0; // Value of the load resistor on the module
float R0 = 10.0; // Sensor resistance in clean air. Will be calculated in setup().

// --- Heating Cycle Timing (in milliseconds) ---
const unsigned long HIGH_HEAT_DURATION = 60000; // 60 seconds for cleaning
const unsigned long LOW_HEAT_DURATION = 90000;  // 90 seconds for sensing
unsigned long cycleStartTime = 0;

// --- State Machine for Heating Cycle ---
enum SensorState { STATE_HIGH_HEAT, STATE_LOW_HEAT };
SensorState currentState = STATE_HIGH_HEAT;

// --- ESP32 PWM (LEDC) Configuration for 1.4V ---
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;
// Calculate PWM duty cycle for 1.4V: (1.4V / 5.0V) * 255 = 71.4
const int LOW_HEAT_PWM_VALUE = 71;


void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Configure heater control pin as output
  pinMode(HEATER_CONTROL_PIN, OUTPUT);

  // Configure ESP32 LEDC (PWM) for low heat phase
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(HEATER_CONTROL_PIN, PWM_CHANNEL);

  Serial.println("ESP32 MQ-7 Sensor Sketch");
  Serial.println("------------------------------------");

  // --- Sensor Preheat Phase ---
  Serial.println("Sensor is preheating for 3 minutes...");
  Serial.println("This allows the sensor to stabilize before calibration.");
  digitalWrite(HEATER_CONTROL_PIN, HIGH); // Full 5V heat
  delay(180000); // 3-minute preheat. For best results, this could be longer.

  // --- Calibration Phase ---
  calibrateSensor();

  // --- Start the Main Cycle ---
  Serial.println("\nStarting main measurement cycle...");
  cycleStartTime = millis();
  // Start the first cycle in HIGH heat mode
  ledcWrite(PWM_CHANNEL, 255); // Full 5V on heater
  currentState = STATE_HIGH_HEAT;
  Serial.println("--> Switched to HIGH heat phase (60s cleaning).");
}


void loop() {
  unsigned long currentTime = millis();

  // --- State Machine Logic ---
  if (currentState == STATE_HIGH_HEAT && (currentTime - cycleStartTime >= HIGH_HEAT_DURATION)) {
    // High heat phase is over, switch to low heat
    currentState = STATE_LOW_HEAT;
    cycleStartTime = currentTime; // Reset timer for the new phase
    ledcWrite(PWM_CHANNEL, LOW_HEAT_PWM_VALUE); // Switch to ~1.4V
    Serial.println("--> Switched to LOW heat phase (90s sensing).");

  } else if (currentState == STATE_LOW_HEAT && (currentTime - cycleStartTime >= LOW_HEAT_DURATION)) {
    // Low heat phase is over. Time to take a reading!
    Serial.print("\n--- Reading at end of low heat cycle ---\n");
    float ppm = getPPM();
    Serial.print("CO Concentration: ");
    Serial.print(ppm);
    Serial.println(" PPM");
    Serial.println("------------------------------------");

    // Switch back to high heat for cleaning
    currentState = STATE_HIGH_HEAT;
    cycleStartTime = currentTime;
    ledcWrite(PWM_CHANNEL, 255); // Back to 5V (255 is max duty cycle)
    Serial.println("--> Switched to HIGH heat phase (60s cleaning).");
  }
}


/**
 * @brief Calibrates the sensor to find R0 in clean air.
 * This runs once at startup after preheating.
 */
void calibrateSensor() {
  Serial.println("Starting calibration... Ensure sensor is in clean air.");
  
  // To calibrate, we need a reading at the end of a low-heat cycle.
  Serial.println("Setting heater to 1.4V for 90s for calibration reading...");
  ledcWrite(PWM_CHANNEL, LOW_HEAT_PWM_VALUE);
  delay(LOW_HEAT_DURATION);

  Serial.println("Taking calibration measurement...");
  float rs_air = 0;
  // Take an average of 10 readings for stability
  for(int i = 0; i < 10; i++) {
    rs_air += readSensorResistance();
    delay(100);
  }
  rs_air /= 10.0;

  // The Rs/R0 ratio in clean air is approximately 9.8 for the MQ-7
  R0 = rs_air / 9.8;

  Serial.println("Calibration complete.");
  Serial.print("Calculated R0: ");
  Serial.println(R0);
  Serial.println("NOTE: You can hard-code this R0 value in the future to skip calibration.");
  Serial.println("------------------------------------");
}


/**
 * @brief Reads the sensor and calculates the CO concentration in PPM.
 * @return The calculated CO concentration in PPM.
 */
float getPPM() {
  float rs = readSensorResistance();
  float ratio = rs / R0;

  // Using the formula from the datasheet's log-log graph: PPM = A * (Rs/R0)^B
  // For CO, a common approximation is: PPM = 100 * (Rs/R0)^-1.5
  if (ratio <= 0) return 0; // Avoid issues with log or pow functions
  float ppm = 100 * pow(ratio, -1.5);
  return ppm;
}


/**
 * @brief Calculates the sensor's current resistance (Rs).
 * @return The calculated resistance in Ohms.
 */
float readSensorResistance() {
  float sensor_volt = readOriginalSensorVoltage();
  if (sensor_volt <= 0) return 0; // Avoid division by zero
  float rs_gas = (5.0 * LOAD_RESISTOR_VALUE / sensor_volt) - LOAD_RESISTOR_VALUE;
  return rs_gas;
}


/**
 * @brief Reads the voltage at the ESP32 pin and calculates the original sensor voltage.
 * @return The sensor's actual output voltage (before the voltage divider).
 */
float readOriginalSensorVoltage() {
  int sensorValue = analogRead(SENSOR_ANALOG_PIN);
  float v_out_divided = (sensorValue / 4095.0) * 3.3;

  // Reverse the voltage divider formula: V_sensor = V_out_divided * (R1 + R2) / R2
  return v_out_divided * (R1_VALUE + R2_VALUE) / R2_VALUE;
}
