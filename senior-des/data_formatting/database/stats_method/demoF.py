import pandas as pd
import matplotlib.pyplot as plt
from scipy.interpolate import UnivariateSpline
import numpy as np

filename = '../data_raw/rider_data/3-7-Neighborhood-Ride-2.csv'
data = pd.read_csv(filename) 
print(data.columns)

data = data.replace([np.inf, -np.inf], np.nan).dropna()

variables = data.columns[1:]
chunk_size_ms = 300  

min_time = data['Time [mS]'].min()
max_time = data['Time [mS]'].max()
num_chunks = ((max_time - min_time) // chunk_size_ms) + 1

start_time = min_time
end_time = max_time
chunk_data = data[(data['Time [mS]'] >= start_time) & (data['Time [mS]'] < end_time)]

chunk_data = chunk_data.replace([np.inf, -np.inf], np.nan).dropna()

plt.figure(figsize=(15, 20))

for j, var in enumerate(variables, start=1):
    ax = plt.subplot(len(variables), 1, j)
    valid_data = chunk_data.dropna(subset=[var])
    
    ax.scatter(valid_data['Time [mS]'], valid_data[var], label='Original Data', color='blue')
    
    # if len(valid_data[var]) > 1:
    #     actual_start_time = valid_data['Time [mS]'].min()
    #     actual_end_time = valid_data['Time [mS]'].max()

    #     spline = UnivariateSpline(valid_data['Time [mS]'].values, valid_data[var].values, s=len(valid_data)*0.5) 
    #     x_new = np.linspace(actual_start_time, actual_end_time, 100)  
    #     y_new = spline(x_new)
        
    #     ax.plot(x_new, y_new, label='Spline Interpolation', color='red')
    
    ax.set_title(f'{var} (Chunk, Time: {start_time} to {end_time} ms)')
    ax.set_xlabel('Time [mS]')
    ax.set_ylabel(var)
    ax.grid(True)
    ax.legend()

plt.subplots_adjust(hspace=0.5, wspace=0.4)
plt.savefig(f'{filename}_chunk_variables.png')
plt.close() 