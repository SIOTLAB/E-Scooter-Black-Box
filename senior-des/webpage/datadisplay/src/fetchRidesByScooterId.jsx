import { initializeApp } from 'firebase/app';
import { getFirestore, collection, query, getDocs } from 'firebase/firestore';

const firebaseConfig = {
    apiKey: "AIzaSyCGveQTchFoLjpjw5nhOktxgizYNp5r8e0",
    authDomain: "e-scooter-blackbox.firebaseapp.com",
    projectId: "e-scooter-blackbox",
    storageBucket: "e-scooter-blackbox.appspot.com",
    messagingSenderId: "987327366278",
    appId: "1:987327366278:web:b8c5977deba5d769c33d07",
    measurementId: "G-SJJFG7LDBK"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const db = getFirestore(app);

function isValidRideData(rideData) {
    const requiredFields = ['date', 'duration', 'escooter', 'location', 'scores', 'storageID', 'time', 'user'];
    const requiredScoreFields = ['bump', 'fall', 'normal', 'wobbly', 'overall'];
    
    // Check for top-level required fields
    for (let field of requiredFields) {
        if (rideData[field] === undefined) {
            console.log(`Validation error: Missing required field '${field}'`);
            return false;
        }
    }

    // Check location fields
    if (!rideData.location || rideData.location.lat === undefined || rideData.location.long === undefined) {
        console.log('Validation error: Incomplete location data');
        return false;
    }

    // Check for necessary subfields in scores
    if (rideData.scores) {
        for (let field of requiredScoreFields) {
            const score = rideData.scores[field];
            if (!score || score.percentage === undefined || (field !== 'overall' && score.rating === undefined)) {
                console.log(`Validation error: Missing or incomplete score data for '${field}'`);
                return false;
            }
        }
    } else {
        console.log('Validation error: Missing entire scores object');
        return false;
    }

    return true;
}

async function fetchRidesByScooterId() {
    const rideDataByScooter = {};

    try {
        const scooterIdsCollection = query(collection(db, 'Scooters'));
        const querySnapshot = await getDocs(scooterIdsCollection);

        for (const scooterDoc of querySnapshot.docs) {
            const scooterId = scooterDoc.id;
            const ridesCollectionPath = `Scooters/${scooterId}/rides`;
            const ridesCollection = query(collection(db, ridesCollectionPath));
            const ridesSnapshot = await getDocs(ridesCollection);

            const ridesData = [];

            ridesSnapshot.forEach((rideDoc) => {
                const rideData = rideDoc.data();
                rideData.escooter = scooterId; // Add the e-scooter ID to the ride data

                if (isValidRideData(rideData)) {
                    ridesData.push(rideData); // Only add if valid
                } else {
                    console.log('Skipped invalid ride data', rideData);
                }
            });

            rideDataByScooter[scooterId] = ridesData;
        }

        console.log(rideDataByScooter); // Log the ride data grouped by e-scooter ID
        return rideDataByScooter;
    } catch (error) {
        console.error("Error fetching ride data:", error);
        return null; // Return null in case of error
    }
}

export default fetchRidesByScooterId;