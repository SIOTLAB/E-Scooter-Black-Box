import csv
import matplotlib.pyplot as plt
from datetime import datetime
import time
import os

# 62KB (10 sec) --> 22320KB (1hr/3600s)= 22.3 Mb/Hr = 0.0217 GB/hr (161.29 HR to hit capacity, then we have buffer)
# Micro SD card space = 3.5GB + 0.2 GB buffer

def parse_csv(file_path):
    timestamps = []
    orientations = []

    with open(file_path, 'r') as file:
        csv_reader = csv.DictReader(file)
        for row in csv_reader:
            # Parse timestamp
            timestamp_str = row['Date']
            timestamp = datetime.strptime(timestamp_str, '%m/%d/%Y %H:%M:%S.%f')
            timestamps.append(timestamp)

            # Parse quaternion data
            qi = float(row['qi'])
            qj = float(row['qj'])
            qk = float(row['qk'])
            qs = float(row['qs'])
            orientations.append((qi, qj, qk, qs))

    return timestamps, orientations

def plot_orientation(timestamps, orientations, save_folder=None, filename=None):
    # Convert quaternions to Euler angles
    euler_angles = [(2 * (qs * qk + qi * qj), 1 - 2 * (qj**2 + qk**2), 2 * (qs * qj - qi * qk)) for qi, qj, qk, qs in orientations]

    # Extract yaw angles
    yaw_angles = [angle[2] for angle in euler_angles]

    # Plot orientation over time
    plt.plot(timestamps, yaw_angles)
    plt.xlabel('Time')
    plt.ylabel('Yaw Angle')
    plt.title('Orientation of the Box Over Time')

    if save_folder and filename:
        if not os.path.exists(save_folder):
            os.makedirs(save_folder)
        
        save_path = os.path.join(save_folder, filename)
        plt.savefig(save_path)

    plt.show()

def plot_data():
    file_path = './data_raw/20240109_174835_MEMS_Sensor_Fusion_Compact.csv' 
    timestamps, orientations = parse_csv(file_path)
    save_folder = 'plots'

    current_time_seconds = time.time()
    current_datetime = datetime.fromtimestamp(current_time_seconds)
    current_time_string = current_datetime.strftime('%Y-%m-%d_%H_%M_%S')
    filename = f'orientation_plot_{current_time_string}.png'
    plot_orientation(timestamps, orientations, save_folder, filename)

if __name__ == "__main__":
    plot_data()