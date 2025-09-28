import pandas as pd
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
import numpy as np

# Read CSV
df = pd.read_csv('sensor_data.csv')

# Scatter plot
plt.figure(figsize=(10,6))
plt.scatter(df['Rs/Ro'], df['PPM'], color='blue', label='Data', s=20)

# Select fitting model, exponential is common for sensors
def exp_model(x, a, b):
    return a * (x ** b)

params, _ = curve_fit(exp_model, df['Rs/Ro'], df['PPM'], p0=[1, -1])

# Fit curve
RsRo_fit = np.linspace(min(df['Rs/Ro']), max(df['Rs/Ro']), 200)
PPM_fit = exp_model(RsRo_fit, *params)

plt.plot(RsRo_fit, PPM_fit, color='red', label='Exponential Fit')
plt.xlabel('Rs/Ro')
plt.ylabel('PPM')
plt.title('Sensor Calibration Curve: Rs/Ro vs PPM')
plt.legend()
plt.grid(True)
plt.show()
