// App.js

import React, { useState, useEffect } from 'react';
import { BrowserRouter as Router, Routes, Route, useNavigate } from 'react-router-dom';
import './App.css';
import { initializeApp } from "firebase/app";
import { getAuth, signInWithEmailAndPassword, onAuthStateChanged, signOut } from "firebase/auth";
import { getFirestore } from "firebase/firestore";
import 'boxicons/css/boxicons.min.css';
import InventoryPage from './components/InventoryPage';
import UserDetailsPage from './components/UserDetailsPage';
import LoginPage from './components/LoginPage';
import 'chart.js/auto'; // Auto-import necessary chart.js features
import HomePage from './components/HomePage';

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
        <Route path="/" element={!loggedIn ? <LoginPage /> : <HomePage />} />
        <Route path="/inventory" element={<InventoryPage />} />
        <Route path="/user-details" element={<UserDetailsPage />} />
      </Routes>
    </Router>
  );
}

export default App;