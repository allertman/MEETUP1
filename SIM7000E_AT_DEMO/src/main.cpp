#include <Arduino.h>                   // we need to include this to indicate we use the Arduino framework
//
//  Meet-up 29 Oct 2019
//  This code shows how to manually communicate from a PC, through the Microcontroller, to a Modemboard
//
#include <HardwareSerial.h>            // include a library so we can define a second serial port
HardwareSerial Modemboard(2);          // Define the second serial port and call it Modemboard, RXpin=16, TXpin=17

void setup() {                         // this routine is called at the start of the ESP32, and runs once
  Serial.begin(9600);                // initialize the standard serial port, connecting the ESP32 with the PC
  Modemboard.begin(115200);            // initialize the second serial port, connecting the ESP32 with the Modemboard
}

void loop() {                          // this routine is called over and over again after setup() is finished
  if (Serial.available()) {            // when data is available in the Serial port from the PC...
    Modemboard.write(Serial.read());   // ...we copy a byte over to the Serial port towards the Modemboard
  }
  if (Modemboard.available()) {        // when data is available in the Serial port from the Modemboard...
    Serial.write(Modemboard.read());   // ...we copy a byte over to the Serial port towards the PC
  }
}