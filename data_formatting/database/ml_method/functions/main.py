from firebase_admin import credentials, firestore, storage, initialize_app
from firebase_functions import firestore_fn
from predict import predict
from io import StringIO
import pandas as pd

cred = credentials.Certificate("./e-scooter-blackbox-firebase-adminsdk-28nm7-e49f916e5f.json")
initialize_app(cred, {
    'storageBucket': 'e-scooter-blackbox.appspot.com'
})

def getFile(document_id, file_name):
    bucket = storage.bucket()
    file_path = f"{document_id}/{file_name}"

    blob = bucket.blob(file_path)
    file_new_name = f"{document_id}-{file_name}"
    blob_bytes = blob.download_as_string()
    blob_string = blob_bytes.decode('utf-8')
    blob_file = StringIO(blob_string)
   
    return blob_file, file_new_name

def updateAllDocs(db):
    db = firestore.client()
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
                
                csv_file, filename= getFile(user.id, storageId)
                scores = predict(csv_file, filename)
                ride_doc.update({'scores': scores})

@firestore_fn.on_document_created(document="Scooters/{scooterId}/users/{userId}/rides/{rideId}")
def update_scores_on_create(event: firestore_fn.Event[firestore_fn.DocumentSnapshot | None]) -> None:
    if event.data is None:
        return
    try:
        storageId = event.data.get('storageID')
        user_id = event.data.get('userID')

    except KeyError:
        return

    csv_file, filename = getFile(user_id, storageId)
    scores = predict(csv_file, filename)

    event.data.reference.update({'scores': scores})