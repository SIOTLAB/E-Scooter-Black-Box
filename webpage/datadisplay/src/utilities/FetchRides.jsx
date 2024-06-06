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
    const requiredFields = ['date', 'duration', 'escooter', 'location', 'scores', 'storageID', 'time'];
    const scoreSubFields = ['bump', 'fall', 'normal', 'wobbly'];  // Removed 'overall' from this list

    // Check for top-level required fields
    for (let field of requiredFields) {
        if (rideData[field] === undefined) {
            console.log(`Validation error: Missing required field '${field}'`);
            return false;
        }
    }

    // Check for necessary subfields in scores
    if (rideData.scores) {
        for (let subField of scoreSubFields) {
            if (!rideData.scores[subField] || rideData.scores[subField].percentage === undefined || rideData.scores[subField].rating === undefined) {
                console.log(`Validation error: Missing or incomplete scores subfield '${subField}'`);
                return false;
            }
        }

        // Special validation for 'overall' since it doesn't follow the same structure
        if (rideData.scores.overall === undefined) {
            console.log('Validation error: Missing overall score');
            return false;
        }

    } else {
        console.log('Validation error: Missing entire scores object');
        return false;
    }
    // Optionally, add more specific checks if 'overall' needs to match certain criteria
    // e.g., check if overall is a number or within a specific range
    return true;
}

async function FetchRides() {
    // Create an object to store ride data grouped by user name
    const rideDataByUser = {};

    try {
        // Create a query to fetch all scooter IDs
        const scooterIdsCollection = query(
            collection(db, 'Scooters')
        );

        // Retrieve the documents from the collection
        const querySnapshot = await getDocs(scooterIdsCollection);

        // Loop through each scooter ID document
        for (const scooterDoc of querySnapshot.docs) {
            const scooterId = scooterDoc.id;

            // For each scooter ID, retrieve the users collection
            const usersCollectionPath = `Scooters/${scooterId}/users`;
            const usersCollection = query(
                collection(db, usersCollectionPath)
            );
            const usersSnapshot = await getDocs(usersCollection);

            // Loop through each user document
            for (const userDoc of usersSnapshot.docs) {
                const userId = userDoc.id;
                const userData = userDoc.data();
                const userName = userData.name;

                // For each user, retrieve the rides collection
                const ridesCollectionPath = `Scooters/${scooterId}/users/${userId}/rides`;
                const ridesCollection = query(
                    collection(db, ridesCollectionPath)
                );
                const ridesSnapshot = await getDocs(ridesCollection);

                // Loop through each ride document
                for (const rideDoc of ridesSnapshot.docs) {
                    // Here you can access each ride document and process it as needed
                    const rideData = rideDoc.data();

                    // Add the user's name and scooter ID to the ride data
                
                    rideData.escooter = scooterId;
                    if (isValidRideData(rideData)) {
                        if (!rideDataByUser[userName]) {
                            rideDataByUser[userName] = [];
                        }
                        rideDataByUser[userName].push(rideData);
                    } else {
                        console.log("Invalid ride data skipped", rideData);
                    }
                }
            }
        }
        // Now rideDataByUser contains ride data grouped by user name
        console.log(rideDataByUser); // Log the ride data grouped by user name
        return rideDataByUser;
    } catch (error) {
        console.error("Error fetching ride data:", error);
        return null; // Return null in case of error
    }
}
export default FetchRides;