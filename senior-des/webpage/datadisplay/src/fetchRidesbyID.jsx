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
async function fetchRidesbyID() {
    const rideDataByScooter = {};

    try {
        // Create a query to fetch all scooter IDs
        const scooterIdsCollection = collection(db, 'Scooters');
        const querySnapshot = await getDocs(scooterIdsCollection);

        // Loop through each scooter ID document
        for (const scooterDoc of querySnapshot.docs) {
            const scooterId = scooterDoc.id;

            // For each scooter ID, retrieve the users collection
            const usersCollectionPath = `Scooters/${scooterId}/users`;
            const usersCollection = collection(db, usersCollectionPath);
            const usersSnapshot = await getDocs(usersCollection);

            // Loop through each user document
            for (const userDoc of usersSnapshot.docs) {
                const userId = userDoc.id;
                const userData = userDoc.data();
                const userName = userData.name;

                // For each user, retrieve the rides collection
                const ridesCollectionPath = `Scooters/${scooterId}/users/${userId}/rides`;
                const ridesCollection = collection(db, ridesCollectionPath);
                const ridesSnapshot = await getDocs(ridesCollection);

                // Loop through each ride document
                for (const rideDoc of ridesSnapshot.docs) {
                    // Here you can access each ride document and process it as needed
                    const rideData = rideDoc.data();

                    // Add the user's name and scooter ID to the ride data
                    rideData.user = userName;
                    rideData.escooter = scooterId;

                    // Group ride data by scooter ID
                    if (!rideDataByScooter[scooterId]) {
                        rideDataByScooter[scooterId] = [];
                    }
                    rideDataByScooter[scooterId].push(rideData);
                }
            }
        }

        // Now rideDataByScooter contains ride data grouped by scooter ID
        console.log("id:", rideDataByScooter); // Log the ride data grouped by scooter ID
        return rideDataByScooter;
    } catch (error) {
        console.error("Error fetching ride data:", error);
        return null; // Return null in case of error
    }
}

export default fetchRidesbyID;