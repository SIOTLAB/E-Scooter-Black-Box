import numpy as np
import csv

def aggregate_scores(data):
    # Convert time from ms to s and prepare for aggregation
    for entry in data:
        entry['time'] = tuple(t / 100 for t in entry['time'])  # Convert ms to s

    # Find the maximum time to set up bins
    max_time = max(entry['time'][1] for entry in data)

    # Set up time bins every 20 seconds
    bucket_size = 20  # in seconds
    time_bins = np.arange(0, max_time + bucket_size, bucket_size)
    aggregated_scores = {i: {'Wobbly': [], 'Bump': []} for i in range(len(time_bins))}

    # Assign scores to the appropriate time bin
    for entry in data:
        time_mid = (entry['time'][0] + entry['time'][1]) / 2
        bin_index = np.digitize(time_mid, time_bins) - 1  # Find bin index, adjust for zero-indexing
        if 'Wobbly' in entry:
            aggregated_scores[bin_index]['Wobbly'].append(entry['Wobbly'])
        if 'Bump' in entry:
            aggregated_scores[bin_index]['Bump'].append(entry['Bump'])

    # Calculate average scores for each bin
    average_scores = []
    for i in range(len(time_bins) - 1):
        avg_wobbly = np.mean(aggregated_scores[i]['Wobbly']) if aggregated_scores[i]['Wobbly'] else None
        avg_bump = np.mean(aggregated_scores[i]['Bump']) if aggregated_scores[i]['Bump'] else None
        average_scores.append({
            'Interval': f"{time_bins[i]}-{time_bins[i+1]} sec",
            'Average Wobbly': avg_wobbly,
            'Average Bump': avg_bump
        })

    return average_scores, aggregated_scores

def read_data_from_file(file_name):
    data = []
    with open(file_name, mode='r') as file:
        csv_reader = csv.reader(file)
        for row in csv_reader:
            if row:  # check if row is not empty
                entry = {
                    "time": (float(row[0]), float(row[1])),
                    row[2]: float(row[3])
                }
                data.append(entry)
    return data

# Use the function and print the results
file_name = 'output.txt'
data = read_data_from_file(file_name)
average_scores, aggregated_scores = aggregate_scores(data)

# Find the largest score and its corresponding time interval
largest_score = max([score for scores in aggregated_scores.values() for score_list in scores.values() for score in score_list if score is not None])
largest_time_interval = next(interval for interval, scores in aggregated_scores.items() if any(score is not None for score_list in scores.values() for score in score_list))

print(f"Largest Score: {largest_score:.2f}")
print(f"Corresponding Time Interval: {average_scores[largest_time_interval]['Interval']}")

# Calculate average values for each category and the scores
average_values = {category: np.mean([score for scores in aggregated_scores.values() for score in scores[category] if score is not None]) for category in ['Wobbly', 'Bump']}
average_score = np.mean([score for scores in aggregated_scores.values() for category_scores in scores.values() for score in category_scores if score is not None])

print("Average Values:")
for category, value in average_values.items():
    print(f"{category}: {value:.2f}")

print(f"Average Score: {average_score:.2f}")
