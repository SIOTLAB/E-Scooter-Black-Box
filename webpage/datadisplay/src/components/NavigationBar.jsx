import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import './NavigationBar.css';

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

export default NavigationBar;
