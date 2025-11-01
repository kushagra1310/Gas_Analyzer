import serial
import time

# ----------------------------
# CONFIGURATION
# ----------------------------
SERIAL_PORT = 'COM3'       # Change to your ESP32 COM port
BAUD_RATE = 115200         # Must match your ESP32 code
OUTPUT_FILE = 'serial_output.txt'

# ----------------------------
# OPEN SERIAL PORT
# ----------------------------
try:
    ser = serial.Serial(
        port=SERIAL_PORT,
        baudrate=BAUD_RATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=1
    )
    print(f"Opened serial port {SERIAL_PORT} at {BAUD_RATE} baud.")
except serial.SerialException as e:
    print("Could not open serial port:", e)
    exit(1)

# ----------------------------
# LOGGING LOOP
# ----------------------------
try:
    # Open file in append mode, UTF-8 encoding for text-safe logging
    with open(OUTPUT_FILE, 'a', encoding='utf-8') as f:
        print(f"Logging serial data to '{OUTPUT_FILE}'...")
        while True:
            if ser.in_waiting:
                # Read a line from serial
                raw_line = ser.readline()

                try:
                    # Try decoding as UTF-8 (text)
                    line = raw_line.decode('utf-8', errors='replace').strip()
                    print(line)
                    f.write(line + '\n')
                except UnicodeDecodeError:
                    # If decoding fails, save raw bytes in hex
                    hex_line = raw_line.hex()
                    print(f"RAW HEX: {hex_line}")
                    f.write(f"RAW HEX: {hex_line}\n")
                except:
                    print("idk")

except KeyboardInterrupt:
    print("\nStopped logging by user.")

finally:
    ser.close()
    print("Serial port closed.")
