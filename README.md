# MEETUP1
Meetup #1 29 Oct 2019

This repository contains the code of the examples used in the Meetup presentation "How to make an IoT device cellular".

The code for this presentation is based / inspired on the work of others like
Andreas Spiess https://github.com/SensorsIot
and 
Timothy Woo https://github.com/botletics

Special thanks to Volodymyr Shymanskyy https://github.com/vshymanskyy for making the tinyGSM library

The MQTT examples use a test instance on cloudMQTT, which over time I might delete. You can in that case make your own broker instance at cloudMQTT(or somewhere else) and change my server URL/Port and login credentials to your own.

You will find more details of this IoT expamples in the "presentation Meetup iot.pdf" file.

I used Platformio en VSC for my programming, if you like to use the Arduino IDE instead you need to copy the content of main.cpp (in the respective src folders) in a new sketch in the Arduino IDE. Make sure you will install the correct libraries in the Arduino IDE also.

