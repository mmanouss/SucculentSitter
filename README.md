# SucculentSitter
Succulent 'Sitter is a cost effective way to keep succulents at their optimal health, providing succulent-specific care suggestions to your plant.

## Abstract

Our motivation behind the “Succulent ‘Sitter” is to provide a cost effective way to keep plants, specifically succulents, at their optimal health, provide succulent-specific care suggestions, and alleviate the difficulty of caring for specific plants. Our main goal is to routinely remind users when to water their succulent based on light, humidity, and temperature data. The “Succulent ‘Sitter” utilizes a sensor-cloud architecture with AWS to succulent statistics and statistic graphs to the user over Wi-Fi. All costly computing is done on the computer driving the Lilygo and interfacing with the cloud for data analytics that allow us to make care predictions. The Lilygo controls our peripherals, like a buzzer that sounds every thirty minutes to deter bugs. The result of our work is a provided countdown to the next estimated watering day and real-time plant statistics on the Lilygo display, as well as real-time statistics and graphs of the different plant history displayed on our AWS server.

## Motivation

For plant owners and gardeners with a wide variety of plant species to look after, it can be difficult to keep up with the needs of every single plant. Even plants that are the same species don’t have the same DNA or reactions to the environment they share. Our motivation is to provide a cost effective way to keep plants, specifically succulents, at their optimal health, provide succulent-specific care suggestions, and alleviate the difficulty of “succulent ‘sitting.”

## Goals

We want to take a different approach to plant care. Instead of routinely reminding users to water their succulents based on its species, and loosely estimating the sunlight levels and water received by the plant, we want to be able to provide this data to the user. Without relying on the user’s perception of acceptable light and water levels for their succulent, our approach is highly accurate and allows plant owners to synthesize the perfect environment for their succulents using real-time environment variables. By taking in humidity, light, and temperature inputs, we can calculate a predicted watering day for the succulent. So, our goals are to easily notify the user about when to water their plant, and to deter pests by using a buzzer to scare them off.

## Assumptions

We are assuming that users of “Succulent ‘Sitter” have a succulent in an environment well suited to that specific plant type, such as 40-50% humidity minimum, high sunlight levels during the day, and warm enough temperatures. 

## Architecture

We use a humidity and temperature sensor, a photoresistor, and a buzzer connected to the Lilygo T-Display to power it. We use WiFi to communicate with AWS and send succulent analytics and data visualizations to the webpage for the user to check in on their succulents when they are away from the system. 


## Deliverables
- A device that detects the plant’s soil, temperature and light conditions to provide a watering countdown on its display, and real-time plant statistics and statistic history in the form of graphs on an AWS webpage 
- A buzzer that makes a sound every 30 minutes to deter pests from the succulent 

## Hardware Used

| Component/Part                  | Quantity               |
|---------------------------------|------------------------|
| Lilygo T-Display                | 1                      |
| Photoresistor                   | 1                      |
| Humidity and Temperature Sensor | 1                      |
| Resistor                        | 1 (10k Ohm)            |
| Breadboard                      | 1                      |
| Buzzer                          | 1                      |