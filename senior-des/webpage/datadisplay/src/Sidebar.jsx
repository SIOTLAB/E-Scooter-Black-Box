// Sidebar.js
import React from 'react';
import { useNavigate } from 'react-router-dom';
import escooterImage from './imgs/escooter.webp'; // Adjust the path as necessary
import pfp from './imgs/default.jpg'; // Adjust the path as necessary
import './Sidebar.css'; // Create and use Sidebar specific CSS
import { getAuth, signOut } from "firebase/auth";
import { initializeApp } from "firebase/app";
import 'boxicons/css/boxicons.min.css';


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




const Sidebar = () => {
  const navigate = useNavigate();

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
        <div id="sidebar" className="sidebar">
          <div className="top">
            <div className="logo">
              <img src={escooterImage} alt="E-scooter Company" className="illustration-image" />
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
            <button className='btn3' onClick={() => navigate('/inventory')}>
              <i className="bx bxs-spreadsheet"></i>
              <span>Inventory</span>
            </button>
          </div>
          <div className="user-card">
            <div className="user-image">
              <img src={pfp} alt="Profile" className="pfp" /> 
              <h2>Raghav B.</h2>
              <p>rbatra@gmail.com</p>
            </div>
            <div className="action-icon">
              <i className="bx bxs-log-out bx-border" onClick={handleLogout}></i>
              <i className="bx bx-cog bx-border "></i>
            </div>
          </div>
        </div>
        </header>
      </div>
  );
};

export default Sidebar;