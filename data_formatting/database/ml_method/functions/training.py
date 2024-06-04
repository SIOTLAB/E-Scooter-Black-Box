import numpy as np
import pandas as pd
import os
import shutil
from io import StringIO
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.metrics import classification_report, accuracy_score
from scipy.signal import welch
from scipy.stats import skew, kurtosis, entropy as sp_entropy
from joblib import dump, load
from sklearn.neural_network import MLPClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.svm import SVC
from sklearn.naive_bayes import GaussianNB
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier

def pre_processing(original_directory):
    base_dir = os.path.dirname(original_directory)
    new_directory = os.path.join(base_dir, 'duplicates')
    os.makedirs(new_directory, exist_ok=True)

    for filename in os.listdir(original_directory):
        if filename.endswith('.csv'):
            src_file_path = os.path.join(original_directory, filename)
            dest_file_path = os.path.join(new_directory, filename)
            shutil.copy(src_file_path, dest_file_path)
    return new_directory

def feature_extraction(filename, data):

    data.columns = data.columns.str.strip()

    columns = ['AccX [mg]', 'AccY [mg]', 'AccZ [mg]', 'GyroX [mdps]', 'GyroY [mdps]', 'GyroZ [mdps]', 'MagX [mgauss]', 'MagY [mgauss]', 'MagZ [mgauss]']
    fs = 100 
    features = {}
    spectral_features = {}
    statistical_features = {}
    for column in columns:
        if 'Acc' in column:
            signal = data[column] / 1000.0
        elif 'Gyro' in column:
            signal = data[column] / 1000.0
        else:
            signal = data[column]

        f, Pxx = welch(signal, fs=fs, nperseg=300)
        centroid = np.sum(f * Pxx) / np.sum(Pxx)
        # entropy = -np.sum(Pxx * np.log2(Pxx))
        time_features = {
            'start_time': data['Time [mS]'].min(),
            'end_time': data['Time [mS]'].max()
        }
        spectral_features = {
            'centroid': centroid,
            'bandwidth': np.sqrt(np.sum((f - centroid)**2 * Pxx) / np.sum(Pxx)),
            'flatness': np.exp(sp_entropy(Pxx)) / np.mean(Pxx),
            'rolloff': f[np.cumsum(Pxx) >= 0.85 * np.sum(Pxx)][0]
        }

        statistical_features = {
            'mean': np.mean(signal),
            'variance': np.var(signal),
            'stddev': np.std(signal),
            'peak_to_peak': np.ptp(signal),
            'rms': np.sqrt(np.mean(np.square(signal))),
            'skewness': skew(signal),
            'kurtosis': kurtosis(signal),
            'zcr': ((signal[:-1] * signal[1:]) < 0).sum() 
        }

        spectral_features_df = pd.DataFrame([spectral_features]).fillna(0)
        statistical_features_df = pd.DataFrame([statistical_features]).fillna(0)
        time_features_df = pd.DataFrame([time_features]).fillna(0)
        
        clean_spectral_features = spectral_features_df.iloc[0].to_dict()
        clean_statistical_features = statistical_features_df.iloc[0].to_dict()
        clean_time_features = time_features_df.iloc[0].to_dict()

        sensor_name = column.split(' ')[0][:-1] 
        features[sensor_name] = {**clean_spectral_features, **clean_statistical_features, **clean_time_features}
    
    if "-" in filename:
        label = filename.split("-")[0]
    else:
        label = filename.split(".")[0] 

    return features, label

def feature_extraction_folder(directory):
    feature_extraction_matrix = []
    labels = []
    for filename in os.listdir(directory):
        data = pd.read_csv(os.path.join(directory, filename))

        chunk_size_ms = 300
        min_time = data['Time [mS]'].min()
        max_time = data['Time [mS]'].max()
        num_chunks = ((max_time - min_time) // chunk_size_ms) + 1
        
        for i in range(int(num_chunks)):
            start_time = min_time + (i * chunk_size_ms)
            end_time = start_time + chunk_size_ms
            chunk_data = data[(data['Time [mS]'] >= start_time) & (data['Time [mS]'] < end_time)]

            features, label = feature_extraction(filename, chunk_data)
            feature_extraction_matrix.append(features)
            labels.append(label)

    return feature_extraction_matrix, labels

def feature_extraction_file(csv_file, filename):
    feature_extraction_matrix = []
    labels = []
    csv_file.seek(0)
    data = pd.read_csv(csv_file, delimiter=',', skipinitialspace=True)
    
    chunk_size_ms = 300
    min_time = data['Time [mS]'].min()
    max_time = data['Time [mS]'].max()
    num_chunks = ((max_time - min_time) // chunk_size_ms) + 1
    
    for i in range(int(num_chunks)):
        start_time = min_time + (i * chunk_size_ms)
        end_time = start_time + chunk_size_ms
        chunk_data = data[(data['Time [mS]'] >= start_time) & (data['Time [mS]'] < end_time)]

        features, label = feature_extraction(filename, chunk_data)
        feature_extraction_matrix.append(features)
        labels.append(label)

    return feature_extraction_matrix, labels
def rf_model(X_train, X_test, y_train, y_test):
    rf = RandomForestClassifier(n_estimators=100, random_state=42)
    rf.fit(X_train, y_train)

    dump(rf, 'rf_model.joblib')
    y_pred_rf = rf.predict(X_test) 

    print("Random Foreest Classification Report: ") 
    print(classification_report(y_test, y_pred_rf))
    print("Random Forest Accuracy:", accuracy_score(y_test, y_pred_rf))

def gradient_booster_model(X_train, X_test, y_train, y_test):
    gb = GradientBoostingClassifier(n_estimators=100, learning_rate=1.0, max_depth=1, random_state=42)
    gb.fit(X_train, y_train)

    dump(gb, 'gb_model.joblib')
    y_pred_gb = gb.predict(X_test) 

    print("Gradient Boosting Classification Report: ") 
    print(classification_report(y_test, y_pred_gb))
    print("Gradient Boosting Accuracy:", accuracy_score(y_test, y_pred_gb))

def knn_model(X_train, X_test, y_train, y_test):
    knn = KNeighborsClassifier(n_neighbors=4)
    
    train = X_train
    nan_columns = train.columns[train.isna().any()].tolist()

    knn.fit(X_train, y_train)

    dump(knn, 'knn_model.joblib')
    y_pred_knn = knn.predict(X_test)

    print("KNN Classification Report: ")
    print(classification_report(y_test, y_pred_knn))
    print("KNN Accuracy:", accuracy_score(y_test, y_pred_knn))

def decision_tree_model(X_train, X_test, y_train, y_test):
    dt = DecisionTreeClassifier(random_state=42)  
    dt.fit(X_train, y_train)  

    dump(dt, 'dt_model.joblib')
    y_pred_dt = dt.predict(X_test) 

    print("Decision Tree Classification Report: ") 
    print(classification_report(y_test, y_pred_dt))
    print("Decision Tree Accuracy:", accuracy_score(y_test, y_pred_dt))

def ann_model(X_train, X_test, y_train, y_test):
    ann = MLPClassifier(random_state=1, max_iter=300, hidden_layer_sizes=(100,100,100,))
    ann.fit(X_train, y_train)

    dump(ann, 'ann_model.joblib')
    y_pred_ann = ann.predict(X_test)

    print("ANN Classification Report: ") 
    print(classification_report(y_test, y_pred_ann))
    print("ANN Accuracy:", accuracy_score(y_test, y_pred_ann))

def logistic_regression_model(X_train, X_test, y_train, y_test):
    lr = LogisticRegression(C=1,random_state=42,max_iter=1000)  
    lr.fit(X_train, y_train)  

    dump(lr, 'lr_model.joblib')
    y_pred_lr = lr.predict(X_test) 

    print("Logistic Regression Classification Report: ") 
    print(classification_report(y_test, y_pred_lr))
    print("Logistic Regression Accuracy:", accuracy_score(y_test, y_pred_lr))

def svm_model(X_train, X_test, y_train, y_test):
    svm = SVC(random_state=42)
    svm.fit(X_train, y_train)

    dump(svm, 'svm_model.joblib')
    y_pred_svm = svm.predict(X_test)

    print("SVM Classification Report: ")
    print(classification_report(y_test, y_pred_svm))
    print("SVM Accuracy:", accuracy_score(y_test, y_pred_svm))

def naive_bayes_model(X_train, X_test, y_train, y_test):
    nb = GaussianNB()
    nb.fit(X_train, y_train)

    dump(nb, 'nb_model.joblib')
    y_pred_nb = nb.predict(X_test)

    print("Naive Bayes Classification Report: ")
    print(classification_report(y_test, y_pred_nb))
    print("Naive Bayes Accuracy:", accuracy_score(y_test, y_pred_nb))

def convert_features_to_df(feature_extraction_matrix, labels):
    df_features = pd.DataFrame(feature_extraction_matrix)
    df_features['label'] = labels

    return df_features

def flatten_df(df):
    flat_df = pd.DataFrame()

    for column in df.columns:
        col_data = pd.DataFrame(index=df.index)
        col_data = df[column].apply(pd.Series)
        
        col_data.columns = [f"{column}_{subcol}" for subcol in col_data.columns]
        flat_df = pd.concat([flat_df, col_data], axis=1)
    
    return flat_df

def ml_models():
    directory = pre_processing('../data_raw/benchmarks/')

    feature_extraction_matrix, labels = feature_extraction_folder(directory)
    df_features = convert_features_to_df(feature_extraction_matrix, labels)

    X_flat = flatten_df(df_features.drop('label', axis=1))
    y = df_features['label']

    X_train, X_test, y_train, y_test = train_test_split(X_flat, y, test_size=0.2, random_state=42)

    knn_model(X_train, X_test, y_train, y_test)
    decision_tree_model(X_train, X_test, y_train, y_test)
    logistic_regression_model(X_train, X_test, y_train, y_test)
    ann_model(X_train, X_test, y_train, y_test)
    svm_model(X_train, X_test, y_train, y_test)
    naive_bayes_model(X_train, X_test, y_train, y_test)
    gradient_booster_model(X_train, X_test, y_train, y_test)
    rf_model(X_train, X_test, y_train, y_test)

if __name__ == '__main__':
    ml_models()