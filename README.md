<h1 align="center"> E-Scooter Rough Riding Detection

  ![Python](https://img.shields.io/badge/python-v3.8+-blue.svg)
  ![C](https://img.shields.io/badge/gcc-v14.38+-pink.svg)
  ![Dependencies](https://img.shields.io/badge/dependencies-up%20to%20date-brightgreen.svg)
  [![GitHub Issues](https://img.shields.io/github/issues/joshuajerome/senior-des.svg)](https://github.com/joshuajerome/senior-des/issues)
  ![Contributions welcome](https://img.shields.io/badge/contributions-welcome-yellow.svg)
  
</h1>

## About

As cities become increasingly congested, ride sharing has become an appealing alternative to traditional modes of transportation. Bicycle and scooter sharing are some of the most popular forms of ride sharing, especially in settings where most customers are looking to travel short distances for personal trips. As with all mobile transporation, cost and security are important factors to consider during usage. Furthermore, it is important to identify users or circumstances that compromise the functionality/safety of the scooter or person respectively. We propose a solution involving a ‘black box’ system that will be able to detect misuse and rough handling of e-bicycles and e-scooters in order to improve the cost-effectiveness and appeal of ride sharing services.

## Usage

First clone this repository, then follow the steps listed under _Sensortile.box Pro Programming_ [here](https://github.com/SIOTLAB/E-Scooter-Black-Box/tree/main/senior-des/STMDataPack)

```shell
git clone git@github.com:SIOTLAB/E-Scooter-Black-Box.git
```

## Data Processing / Machine Learning

The 'data_formatting' folder contains all Python scripts related to data processing and analysis of sensory data.

## Blackbox Gateway Service Provider Web Application

The 'webpage/datadisplay' folder contains all React code for the service provider website for visualizing e-scooter fleet usage and riding metrics.

## Blackbox Gateway iOS Application

The 'BlackboxGatewayiOS' folder contains the source code for the iOS app responsible for connecting to the SensorTile.Box Pro for data offloading.
