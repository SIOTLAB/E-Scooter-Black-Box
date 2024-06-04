// App.js

import React, { useState, useEffect } from 'react';
import { BrowserRouter as Router, Routes, Route, useNavigate } from 'react-router-dom';
import './App.css';
import { initializeApp } from "firebase/app";
import { getAuth, signInWithEmailAndPassword, onAuthStateChanged, signOut } from "firebase/auth";
import { getFirestore } from "firebase/firestore";
import escooterImage from './imgs/escooter.webp';
import 'boxicons/css/boxicons.min.css';
import InventoryPage from './InventoryPage';
import UserDetailsPage from './UserDetailsPage';
import FetchRides from './FetchRides';
import { Pie } from 'react-chartjs-2';
import defpfp from './imgs/default.jpg';
import 'chart.js/auto'; // Auto-import necessary chart.js features

// Your web app's Firebase configuration
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
const auth = getAuth(app);
const db = getFirestore(app);

FetchRides();



const userRideData = FetchRides();

const handlePrint = () => {
  // Attempt to force background graphics in print
  const css = '@page { size: landscape; } body { -webkit-print-color-adjust: exact; }',
      head = document.head || document.getElementsByTagName('head')[0],
      style = document.createElement('style');

  style.type = 'text/css';
  style.media = 'print';

  if (style.styleSheet){
    style.styleSheet.cssText = css;
  } else {
    style.appendChild(document.createTextNode(css));
  }

  head.appendChild(style);

  window.onafterprint = () => head.removeChild(style); // Clean up after print
  window.print();
};

const NavigationBar = () => {
    const navigate = useNavigate();
    const [isPopupVisible, setIsPopupVisible] = useState(false);
  
  
    const togglePopup = () => {
      setIsPopupVisible(!isPopupVisible);
    };
  
    return (
      <nav className="navigation-bar">
        <div className="top-right-sidebar">
          <button id="notificationButton" className="notification-btn" onClick={togglePopup}>
            <i className="bx bxs-bell bx-md"></i>
          </button>
          <button id="printButton" className="settings-btn" onClick={handlePrint}>
            <i className='bx bxs-printer bx-md'></i>
          </button>
        </div>
        {isPopupVisible && (
          <div className="popup">
            <div className="popup-content">
              <h2>Alerts</h2>
              <div className="notifications-header">
                <span>Messages:</span> 
              </div>
              <div className="notification-item">Scooter #3245 is low battery!</div>
              <div className="notification-item">4 New users added this week!</div>
              <div className="notification-item">Increase of scooter usage in Santa Clara!</div>
              <button id="closePopupButton" onClick={togglePopup} className="close">Close</button>
            </div>
          </div>
        )}
      </nav>
    );
  };

  const LoginPage = () => {
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [error, setError] = useState(null);
    const navigate = useNavigate();
  
    const handleLogin = async (e) => {
      e.preventDefault();
      try {
        await signInWithEmailAndPassword(auth, email, password);
        navigate('/');
      } catch (error) {
        setError(error.message);
      }
    };
  
    return (
      <div className="login-page">
        <div className="login-container"> 
          <div className="escooter-logo">
            <img src={escooterImage} alt="Illustrative content" className="illustration-image" />
          </div>
          <form onSubmit={handleLogin} className="login-form">
            <div className="email-input">
              <label htmlFor="email">Username:</label>
              <input
                id="email"
                type="email"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
              />
            </div>
            <div className="password-input">
              <label htmlFor="password">Password:</label>
              <input
                id="password"
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
              />
            </div>
            <button type="submit" className="login-button">Login</button>
            {error && <div className="error-message">{error}</div>}
          </form>
        </div>
      </div>
    );
  };

  const getMostRecentRides = (data) => {
    // Flatten the data array and sort it by date in descending order
    const allRides = Object.entries(data).flatMap(([user, rides]) =>
      rides.map(ride => ({
        user,
        date: ride.date,
        escooter: ride.escooter,
        duration: ride.duration
      }))
    );
    // Sort by date
    allRides.sort((a, b) => new Date(b.date) - new Date(a.date));
    // Return the top 5 most recent rides
    return allRides.slice(0, 5);
  };

  function Home() {
    const navigate = useNavigate();
    const [userData, setUserData] = useState({});
    const [bestTimes, setBestTimes] = useState({});
    const [worstTimes, setWorstTimes] = useState({});

    const [ridesData, setRidesData] = useState({});
    const [mostRecentRides, setMostRecentRides] = useState([]);

    const [pieData, setPieData] = useState({
        labels: ['Normal', 'Bumps', 'Falling Down', 'Wobbly'],
        datasets: [
            {
                data: [], // This will be calculated from userData
                backgroundColor: ['#FFCE56', '#36A2EB', '#FF6384', '#cc65fe'],
                hoverBackgroundColor: ['#FFCE56', '#36A2EB', '#FF6384', '#cc65fe']
            }
        ]
    });
  
    const [averageScore, setAverageScore] = useState(0);

    useEffect(() => {
        async function fetchData() {
            const rideData = await FetchRides(); 
            if (!rideData) {
                // Handle case where rideData is undefined or null
                console.error("Ride data is undefined or null.");
                return;
            }
            setUserData(rideData);
            setRidesData(rideData);
            const recentRides = getMostRecentRides(rideData);
            setMostRecentRides(recentRides);

            // Calculate best and worst times
            let bestTimes = {};
            let worstTimes = {};
            let totalScore = 0;
            let count = 0;

            for (let user in rideData) {
                for (let ride of rideData[user]) {
                    for (let category in ride.scores) {
                        if (!(category in bestTimes) || ride.scores[category].percentage > bestTimes[category].percentage) {
                            bestTimes[category] = {
                                "time": ride.time,
                                "percentage": ride.scores[category].percentage
                            };
                        }
                        if (!(category in worstTimes) || ride.scores[category].percentage < worstTimes[category].percentage) {
                            worstTimes[category] = {
                                "time": ride.time,
                                "percentage": ride.scores[category].percentage
                            };
                        }
                    }
                    totalScore += ride.overall;
                    count++;
                }
            }

            setAverageScore(totalScore / count);

            // Calculate pie chart data
            const scores = { normal: [], bump: [], fall: [], wobbly: [] };
            Object.values(rideData).forEach(user => {
                user.forEach(ride => {
                    scores.normal.push(ride.scores.normal.percentage);
                    scores.bump.push(ride.scores.bump.percentage);
                    scores.fall.push(ride.scores.fall.percentage);
                    scores.wobbly.push(ride.scores.wobbly.percentage);
                });
            });
            const averages = Object.keys(scores).map(key =>
                scores[key].reduce((acc, curr) => acc + curr, 0) / scores[key].length
            );
            setPieData(prevData => ({
                ...prevData,
                datasets: [{ ...prevData.datasets[0], data: averages }]
            }));

            setBestTimes(bestTimes);
            setWorstTimes(worstTimes);
        }
        fetchData();
        console.log("fetching data...");
    }, []); // Run once when component mounts

  

    const handleLogout = async () => {
        try {
            await signOut(auth);
            navigate('/');
        } catch (error) {
            console.error('Error signing out:', error);
        }
    };
  
    return (
      <div className="App">
        <header className="App-header">
          <NavigationBar />
          <div className="sidebar">
            <div className="top">
              <div className="logo">
                <img src={escooterImage} alt="E-scooter Company" />
                <span>E-scooter Company</span>
              </div>
              <button className="btn1" onClick={() => navigate('/')}>
                <i className="bx bxs-home"></i>
                <span>Dashboard</span>
              </button>
              <button className="btn2" onClick={() => navigate('/user-details')}>
                <i className="bx bxs-user-detail"></i>
                <span>User Details</span>
              </button>
              <button className="btn3" onClick={() => navigate('/inventory')}>
                <i className="bx bxs-spreadsheet"></i>
                <span>Inventory</span>
              </button>
            </div>
            <div className="user-card">
              <div className="user-image">
                <img src={defpfp} alt="Profile" />
                <h2>Raghav B.</h2>
                <p>rbatra@gmail.com</p>
              </div>
              <div className="action-icon">
                <i className="bx bxs-log-out bx-border" onClick={handleLogout}></i>
                <i className="bx bx-cog bx-border"></i>
              </div>
            </div>
          </div>
          <div className="main-container">
            <h1>Tracking Dashboard</h1>
            <p>Company name</p>
            <div className="status-container">
              <div className="status-item1">
                <button className="online">
                  <i className="bx bxs-car bx-lg bx-border"></i>
                </button>
                <div className="text-container">
                  <h2>Online</h2>
                  <p>2</p>
                </div>
              </div>
              <div className="status-item2">
                <button className="maintenance">
                  <i className='bx bxs-car bx-lg bx-border'></i>
                </button>
                <div className="text-container">
                  <h2>Maintenance</h2>
                  <p>0</p>
                </div>
              </div>
              <div className="status-item3">
                <button className="offline">
                  <i className='bx bxs-car bx-lg bx-border'></i>
                </button>
                <div className="text-container">
                  <h2>Offline</h2>
                  <p>0</p>
                </div>
              </div>
            </div>
            <div className="data-section">
              <div className="time-lists">
              <div>
                <h2>5 Most Recent Rides</h2>
                <table>
                  <thead>
                    <tr>
                      <th>User</th>
                      <th>Date</th>
                      <th>Escooter</th>
                      <th>Duration</th>
                    </tr>
                  </thead>
                  <tbody>
                    {mostRecentRides.map((ride, index) => (
                      <tr key={index}>
                        <td>{ride.user}</td>
                        <td>{ride.date}</td>
                        <td>{ride.escooter}</td>
                        <td>{ride.duration}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
            <div className="chart-container">
                <h2>Pie Chart of Ride Categories</h2>
                <Pie data={pieData} options={{ maintainAspectRatio: false, layout: {padding: {bottom: 200}}}} />
              </div>
            </div>

          </div>
        </header>
      </div>
    );
  }

function App() {
  const [loggedIn, setLoggedIn] = useState(false);

  useEffect(() => {
    const unsubscribe = onAuthStateChanged(auth, (user) => {
      setLoggedIn(!!user);
    });
    return () => unsubscribe();
  }, []);

  return (
    <Router>
      <Routes>
        <Route path="/" element={!loggedIn ? <LoginPage /> : <Home />} />
        <Route path="/inventory" element={<InventoryPage />} />
        <Route path="/user-details" element={<UserDetailsPage />} />
        {/* Insert other Routes here */}
      </Routes>
    </Router>
  );
}

export default App;