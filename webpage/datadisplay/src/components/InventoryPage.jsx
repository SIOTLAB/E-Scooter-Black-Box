// InventoryPage.js
import React, { useState, useEffect} from 'react';
import Sidebar from './Sidebar'; // Adjust the import path as necessary
import './InventoryPage.css'; // Ensure you have this CSS file for InventoryPage specific styles
import { BrowserRouter, useNavigate } from 'react-router-dom';
import { Pie, Scatter } from 'react-chartjs-2';
import { Chart as ChartJS, CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend } from 'chart.js';
import 'chartjs-adapter-date-fns';
import fetchRidesbyID from '../utilities/fetchRidesbyID';
import NavigationBar from './NavigationBar';



ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,

);


const calculateDailyDurations = (ridesData) => {
  const dailyDurations = {};

  Object.values(ridesData).flat().forEach(ride => {
    const date = new Date(ride.date);
    const formattedDate = date.toISOString().split('T')[0]; // Format as "YYYY-MM-DD"
    const durationParts = ride.duration.split(':');
    const durationInMinutes = parseInt(durationParts[0]) * 60 + parseInt(durationParts[1]) + parseInt(durationParts[2]) / 60;

    if (dailyDurations[formattedDate]) {
      dailyDurations[formattedDate] += durationInMinutes;
    } else {
      dailyDurations[formattedDate] = durationInMinutes;
    }
  });

  return Object.entries(dailyDurations).map(([date, totalDuration]) => ({ x: date, y: totalDuration }));
};


const InventoryPage = () => {
  const [selectedScooter, setSelectedScooter] = useState('3412');
  const [ridesData, setRidesData] = useState({});
  const [cache, setCache] = useState({});
  const [scatterData, setScatterData] = useState({ datasets: [] });
  const [pieData, setPieData] = useState({
    labels: ['Morning (6AM-12PM)', 'Afternoon (12PM-6PM)', 'Evening (6PM-12AM)'],
    datasets: [{
      data: [0, 0, 0],
      backgroundColor: ['#FF6384', '#36A2EB', '#FFCE56'],
      hoverBackgroundColor: ['#FF6384', '#36A2EB', '#FFCE56']
    }]
  });

  useEffect(() => {
    const fetchData = async () => {
        if (selectedScooter && !cache[selectedScooter]) {
            // Only fetch data if it's not already cached
            const rideData = await fetchRidesbyID(selectedScooter);
            console.log("fetched data")
            console.log("ridedate by id: ", rideData)
            setCache(prevCache => ({
                ...prevCache,
                [selectedScooter]: rideData // Store fetched data in cache
            }));
            setRidesData(rideData);
            
        } else if (cache[selectedScooter]) {
            // Use cached data
            setRidesData(cache[selectedScooter]);
        }
    };

    fetchData();
}, [selectedScooter, cache]); 


  const updateChartData = (rides) => {
    const timeRanges = { 'Morning': 0, 'Afternoon': 0, 'Evening': 0 };
    if (rides) {
      rides.forEach(ride => {
        const hour = parseInt(ride.time.split(':')[0]);
        const key = hour < 12 ? 'Morning' : (hour < 18 ? 'Afternoon' : 'Evening');
        timeRanges[key]++;
      });
      const totalRides = rides.length;
      const percentages = [
        parseFloat((timeRanges.Morning / totalRides * 100).toFixed(2)),
        parseFloat((timeRanges.Afternoon / totalRides * 100).toFixed(2)),
        parseFloat((timeRanges.Evening / totalRides * 100).toFixed(2))
      ];
      setPieData({
        labels: ['Morning (6AM-12PM)', 'Afternoon (12PM-6PM)', 'Evening (6PM-12AM)'],
        datasets: [{
          data: percentages,
          backgroundColor: ['#FF6384', '#36A2EB', '#FFCE56'],
          hoverBackgroundColor: ['#FF6384', '#36A2EB', '#FFCE56']
        }]
      });
    }
  };

  const updateScatterData = (rides) => {
    // Object to store total duration per day
    const durationByDate = {};
  
    // Accumulate durations by date
    rides.forEach(ride => {
      const date = new Date(ride.date).toISOString().slice(0, 10); // Standardize the date format
      const durationInMinutes = parseDurationToMinutes(ride.duration);
      if (durationByDate[date]) {
        durationByDate[date] += durationInMinutes; // Add to existing date
      } else {
        durationByDate[date] = durationInMinutes; // Initialize new date
      }
    });
  
    // Convert the object into an array suitable for scatter chart data
    const scatterChartData = Object.entries(durationByDate).map(([date, duration]) => {
      return { x: date, y: duration };
    });
  
    // Sort the data by date to ensure chronological order
    scatterChartData.sort((a, b) => new Date(a.x) - new Date(b.x));
  
    // Set the scatter chart data
    setScatterData({
      datasets: [{
        label: 'Ride Durations',
        data: scatterChartData,
        backgroundColor: 'rgba(75, 192, 192, 1)', // Set alpha to 1 for solid color
        borderColor: 'rgba(75, 192, 192, 1)', // Ensure border color matches background for solid appearance
        borderWidth: 1,
        pointRadius: 5, // Adjust radius as needed for visibility
        showLine: true // Set to false for scatter plot, true if you want to connect dots
      }]
    });
  };


  const handleScooterChange = (event) => {
    setSelectedScooter(event.target.value);
    const selectedRides = ridesData[event.target.value];
    if (selectedRides) {
      updateChartData(selectedRides);
      updateScatterData(selectedRides);
      console.log("scatte data:" , selectedRides);
    }
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
        text: 'Ride Distribution by Time of Day',
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

  


  
  
  const parseDurationToMinutes = (duration) => {
    const [hours, minutes, seconds] = duration.split(':').map(Number);
    return hours * 60 + minutes + seconds / 60;
  };
  


  const scatterOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
        legend: {
            display: false // Usually scatter plots do not need a legend unless multiple datasets
        },
        title: {
            display: true,
            text: 'Total Usage per Day',
            color: '#111',
            font: {
                size: 24,
                weight: 'bold',
                family: 'Poppins, sans-serif'
            }
        },
        tooltip: {
            callbacks: {
                label: function(context) {
                    return `${context.dataset.label}: ${context.raw.y.toFixed(2)} minutes on ${context.raw.x}`;
                }
            }
        }
    },
    scales: {
        x: {
            type: 'time',
            time: {
                unit: 'day',
                tooltipFormat: 'MM/dd/yyyy',
            },
            title: {
                display: true,
                text: 'Date',
                color: '#111',
                font: {
                    size: 16,
                    family: 'Poppins, sans-serif'
                }
            }
        },
        y: {
            title: {
                display: true,
                text: 'Total Duration (Minutes)',
                color: '#111',
                font: {
                    size: 16,
                    family: 'Poppins, sans-serif'
                }
            }
        }
    }
};
  
  return (
    <div className="user-details-page-container">
      <NavigationBar />
      <Sidebar />
      <div className="user-details-page">
        <h1 className='userHeader'>Inventory</h1>
        <select name="escooters" onChange={handleScooterChange} value={selectedScooter}>
          {Object.keys(ridesData).map((scooterId) => (
            <option key={scooterId} value={scooterId}>{scooterId}</option>
          ))}
        </select>
        <div className="chart-container">
          <Pie data={pieData} options={pieOptions} />
          <Scatter data={scatterData} options={scatterOptions} />
        </div>
      </div>
    </div>
  );
};

export default InventoryPage;

