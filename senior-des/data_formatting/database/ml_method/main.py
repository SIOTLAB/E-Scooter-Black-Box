import firebase_admin
from firebase_admin import credentials, firestore, storage, initialize_app
from predict import predict
from io import StringIO

def authenticate():
    cred = credentials.Certificate("/Users/suvassravala/Downloads/e-scooter-blackbox-firebase-adminsdk-28nm7-e49f916e5f.json")
    initialize_app(cred, {
    'storageBucket': 'e-scooter-blackbox.appspot.com'
    })
def getFile(document_id, file_name):
    # Get a reference to the default storage bucket
    bucket = storage.bucket()
    file_path = f"{document_id}/{file_name}"
    blob = bucket.blob(file_path)
    
    blob_string = blob.decode('utf-8')  # Convert bytes to string if needed
    blob_file = StringIO(blob_string)

    return blob_file
def main():
    authenticate()
    db = firestore.client()
    scores = {}
    scooters = db.collection('Scooters')
    scooters_stream = scooters.stream()
    for scooter in scooters_stream:
        scooter_doc = scooters.document(scooter.id)
        users = scooter_doc.collection('users')
        users_stream= users.stream()
        for user in users_stream:
            user_doc = users.document(user.id)
            rides = user_doc.collection('rides')
            rides_stream = rides.stream()
            for ride in rides_stream:
                ride_doc = rides.document(ride.id)
                ride_data = ride_doc.get().to_dict()
                storageId = ride_data['storageID']
                
                csv_file= getFile(ride.id, storageId)
                scores = predict(csv_file) 
                ride_doc.update({'scores': scores})

if __name__ == '__main__':
    main()

