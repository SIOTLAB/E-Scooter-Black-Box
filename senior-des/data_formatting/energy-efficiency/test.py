import pickle


with open('data.pkl', 'rb') as f:
    data = pickle.load(f)

for row in data:
    print(row)
