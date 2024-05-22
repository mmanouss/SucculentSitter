# SucculentSitter
Succulent 'Sitter is a cost effective way to keep succulents at their optimal health, providing succulent-specific care suggestions to your plant.

## Motivation

For plant owners and gardeners with a wide variety of plant species’ to look after, it can be difficult to keep up with the needs of every single plant. Even plants that are the same species don’t have the same DNA or reactions to the environment they share. Our motivation is to provide a cost effective way to keep plants, specifically succulents, at their optimal health, provide succulent-specific care suggestions, and alleviate the difficulty of “succulent ‘sitting.”

## Current Solution

There currently exist plant care apps such as “Planta” that attempt to simplify the process of taking care of plants. These solutions work great for scheduling when to water your plants, estimating how much light your plants are exposed to, and identifying the species of the plant that is shown. However, they can’t take into account the real-time conditions of the plants, only recommendations based on the species type. The apps that take light exposure and watering into account are solely based off of a user’s estimate of how much light the plant gets, and how much they watered it, so they aren’t very accurate unless the user is constantly monitoring the plant’s sun levels on their own and has a very good perception of how much water a succulent needs. 

## Project Goals

We want to take a different approach to plant care. Instead of routinely reminding users to water their succulents based on its species, and loosely estimating the sunlight levels and water received by the plant, we want to be able to provide this data to the user. Without relying on the user’s perception of acceptable light and water levels for their succulent, our approach is highly accurate and allows plant owners to synthesize the perfect environment for their succulents using real-time environment variables. By taking in humidity, light, and temperature inputs, we can calculate a predicted watering day for the succulent. So, our goals are to easily notify the user about when to water their plant, and even with the added perk of pest deterrent by using a speaker to scare pests off. 

## Project Approach

The “Succulent Sitter” will utilize a Sensor-Cloud Architecture. All costly computing will be done on the computer driving the Lilygo T-Display and interfacing with the cloud for data analytics that allow us to make care predictions. The TTGO chip will also be controlling our peripherals such as the LEDs which will be used to display watering status and a buzzer which will sound every thirty minutes to deter bugs. As inputs the TTGO will take information from the access point connecting it to the cloud, a photoresistor to take in light exposure for further data processing, and a humidity and temperature sensor. The output on the display suggests different care tips based on the amount of watering and overall levels of sunlight, with the largest text on the display being a countdown to the next predicted watering day.

## Deliverables
- A device that detects the plant’s soil and light conditions to notify the plant owner through connection with AWS about the optimal watering time
- An LED assortment that’s easy to spot from a distance that indicates the watering status of the plant
- A display that offers helpful tips and a watering-day countdown, based on watering frequency statistics and overall sunlight intake
- A buzzer that makes a sound every 30 minutes to deter pests from the succulent 

## Hardware Used

| Component/Part                  | Quantity               |
|---------------------------------|------------------------|
| Lilygo T-Display                | 1                      |
| Photoresistor                   | 1                      |
| Humidity and Temperature Sensor | 1                      |
| Resistor                        | 1 (10k Ohm)            |
| Breadboard                      | 1                      |
| LEDs                            | 3 (1 red, 1 yellow, 1 green) |
| Buzzer                          | 1                      |
| Battery                         | 1                      |