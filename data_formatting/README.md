# Machine Learning for E-Scooter Ride Classification

This repository contains all the machine learning code needed to run and learn about our method for classifying e-scooter rides. It also includes the data collected by riding around on the e-scooters.

## Repository Structure

### Database Folder

The `database` folder is the primary location for our project's code. Note that the `energy_efficiency` folder was used as a reference for our project based on a similar project, but all of our code is located in the `database` section.

#### Data Raw

The `data_raw` folder contains all the data we collected. It is organized as follows:

- `benchmarks`: Contains the data used for training our models.
- `rider_data` and `test_data`: Contains additional data that was not used in the final model training.

#### Stats Method

The `stats_method` folder represents the code used for our initial interpolation-based approach. We attempted to use a graph matching approach to find similarities for classification. This approach is not used in our final solution but is kept for reference.

#### ML Model

The `ml_model` folder contains the core of our machine learning solution. It's structured to support Firebase Cloud Functions. Inside the `functions` folder, there are three main files:

1. `main.py`: This file is responsible for asynchronously calling the entire process when a new document is added to Firebase. It ensures that the model runs on the new data.

2. `training.py`: Used to train the model and create the ensemble models.

3. `predict.py`: Used to generate classification predictions for new upcoming rides and calculate the score to display on the frontend.
