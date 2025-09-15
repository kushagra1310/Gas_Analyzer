import pandas as pd
import matplotlib.pyplot as plt

# Load CSV with time-only formatted timestamps
df = pd.read_csv('output_with_timestamps.csv')

# Convert Timestamp column to datetime for plotting
df['Timestamp'] = pd.to_datetime(df['Timestamp'], format='%H:%M:%S.%f')

# Calculate averages
avg_resistance = df['Resistance_kOhm'].mean()
avg_voltage = df['VoltageOut'].mean()

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8), sharex=True)

# Resistance plot
ax1.plot(df['Timestamp'], df['Resistance_kOhm'], label='Resistance (kΩ)', marker='o', markersize=3, linewidth=1)
ax1.axhline(y=avg_resistance, color='r', linestyle='--', label=f'Avg Resistance: {avg_resistance:.2f} kΩ')
ax1.set_ylabel('Resistance (kΩ)')
ax1.set_title('MQ-7 Sensor Resistance Over Time')
ax1.legend()
ax1.grid(True)

# Voltage plot
ax2.plot(df['Timestamp'], df['VoltageOut'], label='Voltage Out (V)', color='green', marker='o', markersize=3, linewidth=1)
ax2.axhline(y=avg_voltage, color='orange', linestyle='--', label=f'Avg Voltage: {avg_voltage:.3f} V')
ax2.set_xlabel('Timestamp')
ax2.set_ylabel('Voltage (V)')
ax2.set_title('MQ-7 Sensor Voltage Output Over Time')
ax2.legend()
ax2.grid(True)

plt.xticks(rotation=45)
plt.tight_layout()
plt.show()
