import React from 'react';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';

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
        <div className="login-header">
          <h1>User Login</h1>
        </div>
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