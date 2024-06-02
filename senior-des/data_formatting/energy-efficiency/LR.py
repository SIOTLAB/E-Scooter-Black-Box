# -*- coding: utf-8 -*-

import numpy as np
import pickle
import csv
import random
import datetime
from sklearn import tree
from sklearn.utils import shuffle
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import KFold
from sklearn.model_selection import train_test_split
from sklearn.model_selection import GridSearchCV
from sklearn import svm
from sklearn.metrics import accuracy_score,recall_score,f1_score,precision_score
from sklearn.metrics import confusion_matrix


    
if __name__=="__main__":
    data = pickle.load(open('data.pkl','rb'))

    statistics = data.describe()
    y = data['TtB_class']
    columns = ['TtB_class']
    # columns = ['TtB_class','TtB_class1','Area', 'FID']
    X = data.drop(columns,axis=1)
    n = X.shape[1]
    X = np.array(X)
    
    
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3,random_state=24)
    clf = LogisticRegression(C=1,random_state=42,max_iter=1000)
    model = clf.fit(X_train, y_train)
    ypredict = clf.predict(X_test)
    #evaluate
    acc = accuracy_score(y_test,ypredict)
    cnf_matrix = confusion_matrix(y_test,ypredict)
    f1 = f1_score(y_test, ypredict, average='binary')
    precision = precision_score(y_test, ypredict, average='binary')
    recall = recall_score(y_test, ypredict, average='binary')
#    
    print("X1: accuracy using user specific features = {:.2f}%".format(acc*100))
    print(cnf_matrix)
    print('F1:')
    print(f1)
    print('precision:')
    print(precision)
    print('recall:')
    print(recall)
    
 
    print('OK')
    