import pandas as pd
from datetime import datetime, timedelta

# Load your CSV file
df = pd.read_csv('output.csv')

# Starting timestamp as datetime
start_time_str = "16:05:11.226"
start_time = datetime.strptime(start_time_str, "%H:%M:%S.%f")

# Generate timestamps with 300 ms intervals and format as HH:MM:SS.sss string
df['Timestamp'] = [(start_time + timedelta(milliseconds=300 * i)).strftime("%H:%M:%S.%f")[:-3] for i in range(len(df))]

# Move Timestamp to first column
columns = ['Timestamp'] + [col for col in df.columns if col != 'Timestamp']
df = df[columns]

# Save to new CSV
df.to_csv('output_with_timestamps.csv', index=False)

print("Timestamps added (time only) and saved to output_with_timestamps.csv")
