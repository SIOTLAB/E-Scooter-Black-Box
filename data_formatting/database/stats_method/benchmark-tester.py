
import pandas as pd
import matplotlib.pyplot as plt
from scipy.interpolate import UnivariateSpline
import numpy as np

def interpolate_variables_spline(data, variables):
    splines = {}
    for var in variables:
        if len(data[var]) > 1:
            spline = UnivariateSpline(data['Time [mS]'].values, data[var].values, s=len(data) * 0.5)
            splines[var] = spline
    return splines

benchmark_files = [ '../data_raw/benchmarks/wobbly-benchmark.csv',
                   '../data_raw/benchmarks/bump-benchmark.csv']
benchmark_splines = []
variables = [' AccX [mg]', 'AccY [mg]', 'AccZ [mg]', 'GyroX [mdps]', 'GyroY [mdps]', 'GyroZ [mdps]', 'MagX [mgauss]', 'MagY [mgauss]', 'MagZ [mgauss]']

for file in benchmark_files:
    benchmark_data = pd.read_csv(file)
    benchmark_splines.append(interpolate_variables_spline(benchmark_data, variables))

def score_against_benchmarks_splines(data_splines, benchmark_splines, variables, x_new):
    scores = []
    for benchmark in benchmark_splines:
        variable_scores = []
        for var in variables:
            if var in data_splines and var in benchmark:
                data_y = data_splines[var](x_new)
                benchmark_y = benchmark[var](x_new)
                score = np.mean(np.abs(benchmark_y - data_y) / (np.abs(np.max(benchmark_y) - np.min(benchmark_y))))
                score = score - int(score)
                score = 1 - score
                variable_scores.append(score)
        scores.append(np.mean(variable_scores))
    return scores

filename = '../data_raw/rider_data/3-6-30minRide.csv'
new_data = pd.read_csv(filename)
chunk_size_ms = 300
min_time = new_data['Time [mS]'].min()
max_time = new_data['Time [mS]'].max()
num_chunks = ((max_time - min_time) // chunk_size_ms) + 1
high_similarity_instances = []

for i in range(int(num_chunks)):
    start_time = min_time + (i * chunk_size_ms)
    end_time = start_time + chunk_size_ms
    chunk_data = new_data[(new_data['Time [mS]'] >= start_time) & (new_data['Time [mS]'] < end_time)]
    
    data_splines = interpolate_variables_spline(chunk_data, variables)
    
    if data_splines:  
        x_new = np.linspace(start_time, end_time, 300)  
        scores = score_against_benchmarks_splines(data_splines, benchmark_splines, variables, x_new)
        for j, score in enumerate(scores):
            if score > 0.7:
                high_similarity_instances.append((start_time, end_time, j + 1, score))
for instance in high_similarity_instances:
    benchmark ={
        1: "Wobbly",
        2: "Bump"
    }
    print(f"{instance[0]},{instance[1]},{benchmark[instance[2]]},{instance[3]:.2f}")
