// UserDetailsPage.js
import Sidebar from './Sidebar'; // Ensure this is the correct path
import './UserDetailsPage.css'; // Make sure you have the corresponding CSS file
import React, { useState, useEffect} from 'react';
import { BrowserRouter, useNavigate } from 'react-router-dom';
import FetchRides from './FetchRides';
import { Chart as ChartJS, CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend } from 'chart.js';
import { Pie, Bar, Line} from 'react-chartjs-2';

// Registering the components necessary for the line chart
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const rideDataCache = {};

 // State to hold the currently selected user
 const UserDetailsPage = () => {
  // State to hold the currently selected user
  const [selectedUser, setSelectedUser] = useState('Rick Flick');
  // State to hold the user ride data
  const [userRideData, setUserRideData] = useState({});

  useEffect(() => {
    const fetchData = async () => {
      try {
        if (!rideDataCache['data']) {  // Check if data is already cached
          const data = await FetchRides();
          rideDataCache['data'] = data;  // Cache the fetched data
          setUserRideData(data);
        } else {
          setUserRideData(rideDataCache['data']);  // Use cached data
        }
      } catch (error) {
        console.error('Error fetching ride data:', error);
      }
    };
    fetchData();
    console.log("fetchData");
  }, []); 
  
  // Sorting user ride data by date
  if (userRideData[selectedUser]) {
    userRideData[selectedUser].sort((a, b) => new Date(a.date) - new Date(b.date));
  }
  
  const lineChartData = {
    labels: userRideData[selectedUser]?.map(ride => ride.date),
    datasets: [{
      label: 'Overall Score',
      data: userRideData[selectedUser]?.map(ride => ride.scores.overall),
      borderColor: 'rgb(34, 139, 34)',
      backgroundColor: 'rgba(34, 139, 34, 0.2)',
      borderWidth: 2,
      pointBackgroundColor: 'rgb(34, 139, 34)',
      pointBorderColor: '#fff',
      pointHoverBackgroundColor: '#fff',
      pointHoverBorderColor: 'rgb(34, 139, 34)'
    }]
  };
  
  const options = {
    responsive: true,
    scales: {
      y: {
        beginAtZero: false,
        title: {
          display: true,
          text: 'Score',
          color: '#111',
          font: {
            size: 16,
            family: 'Poppins, sans-serif'
          }
        },
        ticks: {
          color: '#111',
          font: {
            size: 14,
            family: 'Poppins, sans-serif'
          }
        }
      },
      x: {
        title: {
          display: true,
          text: 'Date',
          color: '#111',
          font: {
            size: 16,
            family: 'Poppins, sans-serif'
          }
        },
        ticks: {
          color: '#111',
          font: {
            size: 14,
            family: 'Poppins, sans-serif'
          }
        }
      }
    },
    plugins: {
      legend: {
        position: 'top',
        labels: {
          color: 'rgb(34, 139, 34)',
          font: {
            size: 18,
            family: 'Poppins, sans-serif'
          }
        }
      },
      title: {
        display: true,
        text: 'Overall Score by Date',
        color: '#111',
        font: {
          size: 24,
          weight: 'bold',
          family: 'Poppins, sans-serif'
        }
      },
      tooltip: {
        bodyColor: '#ffffff',
        titleColor: '#ffffff',
        borderColor: '#ffffff',
        borderWidth: 1,
        backgroundColor: 'rgba(0, 0, 0, 0.8)',
        bodyFont: {
          family: 'Poppins, sans-serif'
        },
        titleFont: {
          family: 'Poppins, sans-serif'
        },
        callbacks: {
          label: function(tooltipItem) {
            return `Overall Score: ${tooltipItem.raw.toFixed(2)}`; // Format to 2 decimal places
          }
        }
      }
    }
  };
  

  const scoreSums = {
    normal: { total: 0, count: 0 },
    fall: { total: 0, count: 0 },
    wobbly: { total: 0, count: 0 },
    bump: { total: 0, count: 0 }
  };

  userRideData[selectedUser]?.forEach(ride => {
    if (ride.scores.normal) {
      scoreSums.normal.total += ride.scores.normal.percentage;
      scoreSums.normal.count++;
    }
    if (ride.scores.fall) {
      scoreSums.fall.total += ride.scores.fall.percentage;
      scoreSums.fall.count++;
    }
    if (ride.scores.wobbly) {
      scoreSums.wobbly.total += ride.scores.wobbly.percentage;
      scoreSums.wobbly.count++;
    }
    if (ride.scores.bump) {
      scoreSums.bump.total += ride.scores.bump.percentage;
      scoreSums.bump.count++;
    }
  });

  const pieChartData = {
    labels: ['Normal', 'Fall', 'Wobbly', 'Bump'],
    datasets: [{
      label: 'Average Percentage Scores',
      data: [
        scoreSums.normal.count ? (scoreSums.normal.total / scoreSums.normal.count) : 0,
        scoreSums.fall.count ? (scoreSums.fall.total / scoreSums.fall.count) : 0,
        scoreSums.wobbly.count ? (scoreSums.wobbly.total / scoreSums.wobbly.count) : 0,
        scoreSums.bump.count ? (scoreSums.bump.total / scoreSums.bump.count) : 0
      ],
      backgroundColor: [
        'rgb(54, 162, 235)', // Solid blue for Normal
        'rgb(255, 206, 86)', // Solid yellow for Fall
        'rgb(255, 99, 132)', // Solid red for Wobbly
        'rgb(75, 192, 192)'  // Solid teal for Bump
      ],
      borderColor: [
        'rgba(54, 162, 235, 1)',
        'rgba(255, 206, 86, 1)',
        'rgba(255, 99, 132, 1)',
        'rgba(75, 192, 192, 1)'
      ],
      borderWidth: 1
    }]
  };
  
  const pieOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'top',
        labels: {
          color: '#111',
          font: {
            size: 14,
            family: 'Poppins, sans-serif'
          }
        }
      },
      title: {
        display: true,
        text: 'Average Score Percentages by Category',
        color: '#111',
        font: {
          size: 24,
          weight: 'bold',
          family: 'Poppins, sans-serif'
        }
      },
      tooltip: {
        callbacks: {
          label: function(tooltipItem) {
            return `${tooltipItem.label}: ${tooltipItem.raw.toFixed(2)}%`;
          }
        }
      }
    }
  };

  const barChartData = {
    labels: userRideData[selectedUser]?.map(ride => ride.date),
    datasets: [{
      label: 'Duration in Minutes',
      data: userRideData[selectedUser]?.map(ride => {
        const parts = ride.duration.split(':');
        return parseInt(parts[0]) * 60 + parseInt(parts[1]) + parseInt(parts[2]) / 60;
      }),
      backgroundColor: 'rgb(0, 123, 255)',
      borderColor: 'rgb(0, 123, 255)',
      borderWidth: 1
    }]
  };

  const barOptions = {
    scales: {
      y: {
        beginAtZero: true,
        title: {
          display: true,
          text: 'Duration (minutes)',
          font: {
            family: 'Poppins, sans-serif',
            size: 16,
            weight: 'bold',
            color: '#111' // Dark gray color for Y-axis title
          }
        },
        ticks: {
          color: '#111', // Dark gray color for Y-axis ticks
          font: {
            size: 14,
            family: 'Poppins, sans-serif'
          }
        }
      },
      x: {
        title: {
          display: true,
          text: 'Date',
          font: {
            family: 'Poppins, sans-serif',
            size: 16,
            weight: 'bold',
            color: '#111' // Dark gray color for X-axis title
          }
        },
        ticks: {
          color: '#111', // Dark gray color for X-axis ticks
          font: {
            size: 14,
            family: 'Poppins, sans-serif'
          }
        }
      }
    },
    plugins: {
      legend: {
        display: true,
        position: 'top',
        labels: {
          font: {
            family: 'Poppins, sans-serif',
            size: 14,
            weight: 'bold',
            color: '#111' // Dark gray color for legend text
          }
        }
      },
      title: {
        display: true,
        text: 'Duration by Date',
        font: {
          size: 24,
          family: 'Poppins, sans-serif',
          weight: 'bold',
          color: '#111' // Dark gray color for chart title
        }
      },
      tooltip: {
        enabled: true,
        mode: 'index',
        intersect: false,
        callbacks: {
          label: function(tooltipItem) {
            return `${tooltipItem.dataset.label}: ${tooltipItem.raw.toFixed(2)} minutes`;
          }
        }
      }
    },
    responsive: true,
    maintainAspectRatio: false
  };
  
  
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

 // Handle changing the selected user from the dropdown
 const handleUserChange = (event) => {
   setSelectedUser(event.target.value);
 };

 return (
   <div className="user-details-page-container">
     <Sidebar />
     <NavigationBar />
     <div className="user-details-page">
       <h1 className='userHeader'>User Details</h1>
       <select name="users" id="user-select" onChange={handleUserChange} value={selectedUser}>
         {Object.keys(userRideData).map(user => (
           <option key={user} value={user}>{user}</option>
         ))}
       </select>
       <div className="chart-container">
          <Pie data={pieChartData} options={pieOptions} />
          <Line data={lineChartData} options={options} />
          <Bar data={barChartData} options={barOptions} />
      </div>
     </div>
   </div>
 );
};

export default UserDetailsPage;
