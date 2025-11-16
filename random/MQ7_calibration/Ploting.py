import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# --- Configuration ---
CSV_FILENAME = 'calibration_data.csv'

def plot_sensor_curve(filename):
    """
    Reads the calibration data from a CSV file and plots it on a log-log scale,
    which is standard for gas sensor characteristic curves. It also fits a line
    to find the sensor's characteristic equation.
    """
    try:
        # Read the CSV file. The '#' character is used to ignore the final Ro value line.
        data = pd.read_csv(filename, comment='#')
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
        print("Please make sure the CSV file is in the same directory as this script.")
        return
    except pd.errors.EmptyDataError:
        print(f"Error: The file '{filename}' is empty. No data to plot.")
        return

    # Ensure the required columns exist
    if 'ppm' not in data.columns or 'rs/ro' not in data.columns:
        print("Error: CSV file must contain 'ppm' and 'rs/ro' columns.")
        return

    # Extract data
    ppm = data['ppm']
    rs_ro = data['rs/ro']

    # --- Plotting ---
    plt.style.use('seaborn-v0_8-whitegrid')
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot the raw data points
    ax.scatter(ppm, rs_ro, label='Measured Data Points', color='royalblue', zorder=5)

    # --- Curve Fitting on Log-Log Scale ---
    # The relationship is typically linear on a log-log plot (y = mx + c)
    # which corresponds to a power law in linear scale (Rs/Ro = a * PPM^m)
    if len(ppm) >= 2:
        log_ppm = np.log10(ppm)
        log_rs_ro = np.log10(rs_ro)
        
        # Fit a 1st degree polynomial (a line) to the log-transformed data
        m, c = np.polyfit(log_ppm, log_rs_ro, 1)

        # Generate points for the trendline
        fit_line_log = m * log_ppm + c
        fit_line = 10**fit_line_log
        
        ax.plot(ppm, fit_line, color='crimson', linestyle='--', label=f'Log-Log Fit')
        
        # Display the equation of the line
        # The equation is in the form: log10(y) = m*log10(x) + c
        print(f"Sensor Characteristic Equation (Log-Log scale):")
        print(f"log10(rs/ro) = {m:.4f} * log10(PPM) + {c:.4f}")
        

    # Set scale to logarithmic for both axes
    ax.set_xscale('log')
    ax.set_yscale('log')

    # --- Labels and Title ---
    ax.set_title('MQ-7 Sensor Calibration Curve (CO)', fontsize=16)
    ax.set_xlabel('CO Concentration (PPM)', fontsize=12)
    ax.set_ylabel('Rs / Ro Ratio', fontsize=12)
    ax.legend()
    ax.grid(True, which="both", ls="--")

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    plot_sensor_curve(CSV_FILENAME)
