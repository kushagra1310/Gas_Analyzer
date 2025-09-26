import serial
import csv

ser = serial.Serial('COM8', 115200)
with open('output.csv', 'w', newline='') as f:
    writer = csv.writer(f)
    while True:
        line = ser.readline().decode().strip()
        # Process line as needed (split or direct write)
        # For example, write raw lines:
        writer.writerow([line])