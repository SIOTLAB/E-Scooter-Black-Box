# -*- coding: utf-8 -*-


import pandas as pd
import numpy as np
import pickle
from sklearn.model_selection import KFold
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import train_test_split # Import train_test_split function
from sklearn.metrics import mean_squared_error
from sklearn.metrics import accuracy_score,recall_score,f1_score,precision_score
from sklearn.metrics import confusion_matrix


if __name__=="__main__":
    data = pickle.load(open('data.pkl','rb'))

    statistics = data.describe()
    y = data['TtB_class']
    columns = ['TtB_class']
    X = data.drop(columns,axis=1)
    n = X.shape[1]
    X = np.array(X)
    
    
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3,random_state=24)
    
    clf = MLPClassifier(random_state=1, max_iter=300, hidden_layer_sizes=(100,100,100,)).fit(X_train, y_train)
    ypredict = clf.predict(X_test)
    
    #evaluate
    acc = accuracy_score(y_test,ypredict)
    cnf_matrix = confusion_matrix(y_test,ypredict)
    f1 = f1_score(y_test, ypredict, average='binary')
    precision = precision_score(y_test, ypredict, average='binary')
    recall = recall_score(y_test, ypredict, average='binary')
#    
    print("X1: accuracy = {:.2f}%".format(acc*100))
    print(cnf_matrix)
    print('F1:')
    print(f1)
    print('precision:')
    print(precision)
    print('recall:')
    print(recall)
    
    print("OK")