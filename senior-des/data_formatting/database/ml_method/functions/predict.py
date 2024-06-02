import numpy as np
import pandas as pd
import os
import shutil
from training import pre_processing, feature_extraction_file, convert_features_to_df, flatten_df
from joblib import load
from statistics import mode
from collections import Counter
def post_processing(breakdown_percentages):
    labels = ['normal', 'bump', 'wobbly']
    for label in labels:
        if label not in breakdown_percentages:
            breakdown_percentages[label] = 0
    breakdown_percentages['normal']+=(0.67 * breakdown_percentages['bump'])
    breakdown_percentages['bump'] = (0.33 * breakdown_percentages['bump'])
    return breakdown_percentages

def rough_riding_breakdown(knn, dt, rf, gb):
    breakdown = []
    score_val = 0.0
    for i in range(len(knn)):
        predictions = [dt[i], rf[i], knn[i], gb[i]]
        breakdown.append(mode(predictions))
    print(breakdown)
    i = 0
    model_list = [knn, dt, rf, gb, breakdown]
    model_names = ['knn', 'dt', 'rf', 'gb','breakdown']
    
    for model in model_list:
        label_counts = Counter(model)
        total_items = len(model)

        print(f"Percentage breakdown of each label: {model_names[i]} \n")
        i+=1
        for label, count in label_counts.items():
            percentage = (count / total_items) * 100
            print(f"{label}: {percentage:.2f}%")
    label_counts = Counter(breakdown)
    total_items = len(breakdown)
    breakdown_percentages = {}
    print(f"Percentage breakdown of each label: breakdown\n")
    for label, count in label_counts.items():
        percentage = (count / total_items) * 100
        print(f"{label}: {percentage:.2f}%")
        breakdown_percentages[label] = percentage

    score_val = score_val/len(breakdown)
    return breakdown_percentages

def score_generator(breakdown):
    breakdown_percentages = post_processing(breakdown)
    benchmark_scores = {'normal': 79, 'fall': 7, 'wobbly': 3, 'bump': 11}
    deduction = 0
    scores = {}
    for label in breakdown_percentages:
        scores[label] = {}
        scores[label]['percentage'] = breakdown_percentages[label]
        
        if breakdown_percentages[label] < (0.7 * benchmark_scores[label]):
            scores[label]['rating'] = 'low'
            if label == 'normal':
                deduction+=2
        elif breakdown_percentages[label] > (1.3 * benchmark_scores[label]):
            scores[label]['rating'] = 'high'
            if label != 'normal':
                deduction+=2
        else:
            scores[label]['rating'] = 'medium'
    
    scores['overall'] = (breakdown_percentages['normal'] - deduction)
    return scores
def process_new_data(csv_file, filename):
    feature_extraction_matrix, _ = feature_extraction_file(csv_file, filename)
    df_features = convert_features_to_df(feature_extraction_matrix, [None]*len(feature_extraction_matrix))
    X_flat = flatten_df(df_features)
    return X_flat

def load_and_predict(model_file, new_data):
    model = load(model_file)
    predictions = model.predict(new_data)
    return predictions

def predict_new_data(csv_file, filename, model_file):
    new_data_prepared = process_new_data(csv_file, filename)
    predictions = load_and_predict(model_file, new_data_prepared)
    return predictions

def predict(csv_file, filename):
    
    model_file_knn = 'knn_model.joblib'
    model_file_dt = 'dt_model.joblib'
    model_file_rf = 'rf_model.joblib'
    model_file_gb = 'gb_model.joblib'

    predictions_dt = predict_new_data(csv_file, filename, model_file_dt)
    predictions_knn = predict_new_data(csv_file, filename, model_file_knn)
    predictions_rf = predict_new_data(csv_file, filename, model_file_rf)
    prediction_gb = predict_new_data(csv_file, filename, model_file_gb)

    # print("KNN Predictions:", predictions_knn)
    # print("Decision Tree Predictions:", predictions_dt)
    # print("ANN Predictions:", predictions_ann)
    # print("Logistic Regression Predictions:", predictions_lr)
    # print("SVM Predictions:", predictions_svm)
    # print("Naive Bayes Predictions:", predictions_nb)
    # print("")

    breakdown = rough_riding_breakdown(predictions_knn, predictions_dt, predictions_rf, prediction_gb)
    print(breakdown)
    scores = score_generator(breakdown)
    print(scores)
    return scores
if __name__ == '__main__':
    predict(filename="Sens005.csv")