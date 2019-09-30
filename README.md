# Wely

This project is about creating an Internet of Things based weighting lysimeter (named Wely, pronounced weh·
lee). 

A weighing lysimeter is an instrument used to record the mass of something over time. It has often been used for plant science. It's a great way to monitor the water content of a potted plant. For example, the data is recorded with a unit of grams, which is easy to understand and use in an irrigation system. Since the sensor is not in contact with any liquid it does not corrode. Unlike some other sensing techniques, lysimetry takes a reliable sample of the entire pot rather than only one spot inside of it.

As of September, 2019 the Wely sends data to the MATLAB-integrated service ThingSpeak. It has also been integrated into Losant through a Particle Integration, which requires no change to the current code. Data from devices which publish publicly in ThingSpeak can be viewed on ThingSpeak.com by looking for public channels with the tag "wely".

The device is currently built around a Particle Photon and code has been compiled and flashed using their online Integrated Develpment Environment. 

Development started in the summer of 2018 by Phytochem Consulting in Vancouver, BC, Canada. This code repository has been created to facilitate more advanced development involving multiple authors and additional features.

As of September 2019 16 devices have been deployed and are being used to monitor plants in various fashions. This open source repository is composed of the wiring, components, cut files for the housing and code required to build a device. The developer must calibrate the load cell for zero offset, response and temperature compensation.

In 2019 a beta group with four users was run for 6 months. Each user tried out a device and gave two interviews spread out over the testing period. The top features requested were (feature (# times requested)):

    - Notification based on mass setpoint (3)
    - Automatic watering (2)
    - Mode for immediate feedback (2)
    - Battery life indication (2)
    - Higher weight capacity
    - Waterproofing to protect against pot runoff
    - More control over data - open API and source code.
    - Humidity sensing
    - Easier to replace/recharge batteries

