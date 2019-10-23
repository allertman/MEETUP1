#include <Arduino.h>
//
//  Meet-up 29 Oct 2019
//  This code shows how we can build a real IoT device sending temp and humidity info every 10 seconds and switch a lamp on demand
//
#include <HardwareSerial.h>
HardwareSerial Modemboard(2);

#include "Adafruit_Sensor.h"                                 // include sensor library for DHT11 sensor
#include "DHT.h"                                             // include sensor library for DHT11 sensor
DHT dht(13, 11);                                             // DHT11 sensor is connected to pin 13 and DHT type is 11

char replybuffer[255];                                       // gobal array to be reused in sendCommand() and loop()
unsigned long start=0;                                       // used in loop function to time 10 seconds

bool sendCommand(const char *command, const char *reply, uint16_t timeout) {
  while(Modemboard.available()) Modemboard.read();           // clear input buffer if there is any data from before...
  Modemboard.println(command);                               // sent command to modem
  Serial.print(command);                                     // show command we sent to modem
  uint8_t idx = 0;
  bool Match = false;
  unsigned long timer = millis();
  while (!Modemboard.available() && millis() - timer < timeout); // wait for data to arrive
  while (!Match && millis() - timer < timeout) {
    while (Modemboard.available()) {                         // while data is available copy it to replybuffer
      replybuffer[idx] = Modemboard.read();
      idx++;
    }
    if (strstr(replybuffer, reply) != NULL) Match = true;    // check for match
  }
  replybuffer[idx] = '\0';                                   // 0 terminate the string
  Serial.println(replybuffer);                               // show reply from modem
  if (Match) return true;                                    // return 1 if there is a match...
  else return false;                                         // ...or return 0 if there is no match
}


void setup() {                                               // this routine is called at the start of the ESP32, and runs once
  Serial.begin(115200);                                      // initialize the standard serial port, connecting the ESP32 with the PC
  Modemboard.begin(115200);                                  // initialize the second serial port, connecting the ESP32 with the Modemboard

  dht.begin();

  pinMode(LED_BUILTIN,OUTPUT);                               // specify pin 5 as output pin, LED and relay are connected here
  digitalWrite(LED_BUILTIN, HIGH);                           // lamp off at start

  for (int i=0 ; i<10 ; i++) Modemboard.println("AT");       // allow modem to autobaud 
  Modemboard.println("AT&F");                                // set factory defaults
  for (int i=0 ; i<10 ; i++) Modemboard.println("AT");       // allow modem to autobaud

  sendCommand("AT", "OK", 2000);                             // AT to check connection
  sendCommand("ATE0", "OK", 2000);                           // echo off
  sendCommand("AT+CFUN=0", "+CPIN: NOT READY", 5000);        // flight mode on
  sendCommand("AT+COPS=1,2,\"20404\",7", "OK", 5000);        // CATM1 VF
  //sendCommand("AT+COPS=1,2,\"20408\",7", "OK", 5000);      // CATM1 KPN
  sendCommand("AT+CNMP=38", "OK", 5000);                     // use LTE network only
  sendCommand("AT+CMNB=1", "OK", 2000);                      // 1 is for CATM1, 2 is for NB-IoT 
  sendCommand("AT+CFUN=1", "SMS Ready", 5000);               // flight mode on, start network search
  while(!sendCommand("AT+CREG?", ",1", 2000)) ;              // check network registration status
  sendCommand("AT+CAPNMODE=0", "OK", 2000);                  // get APN defintions from the network!
  sendCommand("AT+CGATT?", "OK", 2000);                      // check attach status
  sendCommand("AT+CNACT?", "OK", 5000);                      // check PDP context status
  sendCommand("AT+CNACT=1", "+APP PDP: A", 5000);            // activate PDPcontext
  sendCommand("AT+CNACT?", "OK", 5000);                      // check PDP context, now we should have our IP address

  sendCommand("AT+SMCONF=\"URL\",\"farmer.cloudmqtt.com\",16633", "OK", 2000);  // MQTT config 
  sendCommand("AT+SMCONF=\"CLIENTID\",\"SIM7000\"", "OK", 2000);                // MQTT config
  sendCommand("AT+SMCONF=\"USERNAME\",\"eyneiyga\"", "OK", 2000);               // MQTT config
  sendCommand("AT+SMCONF=\"PASSWORD\",\"_sTKZQbfemKK\"", "OK", 2000);           // MQTT config
  sendCommand("AT+SMDISC", "OK", 2000);                      // make sure old broker connections are released
  sendCommand("AT+SMCONN", "OK", 2000);                      // connect to broker
  sendCommand("AT+SMSTATE?", "OK", 2000);                    // check connection to Broker
  sendCommand("AT+SMSUB=\"lamp\",0", "OK", 2000);            // subscribe to lamp topic
  sendCommand("AT+SMPUB=\"test\",3,0,0", ">", 2000);         // unclear why, but SIM7000 likes to publish before it receives stuff...
  Modemboard.print("hoi");                                   // send content of the message
}

void loop() {                                                // this routine is called over and over again after setup() is finished
  if (Serial.available()) {                                  // when data is available in the Serial port from the PC...
    Modemboard.write(Serial.read());                         // ...we copy a byte over to the Serial port towards the Modemboard
  }
  if (Modemboard.available()) {                              // when data is available in the Serial port from the Modem,...
    uint8_t idx = 0;                                         //      ... we will show it and check for specific commands
    while (Modemboard.available()) {                         // while data is available copy all of it to a replybuffer
      replybuffer[idx] = Modemboard.read();
      idx++;
    }
    replybuffer[idx] = '\0';                                 // terminate string to avoid garbage in print
    Serial.print(replybuffer);                               // show all data from Modem on Serial of the PC
    if (strstr(replybuffer, "\"lamp\",\"0\"") != NULL) {     // if we receive +SMSUB: "lamp","0" ...
      digitalWrite(LED_BUILTIN, HIGH);                       // ...swith lamp off
    }
    if (strstr(replybuffer, "\"lamp\",\"1\"") != NULL) {     // if we receive +SMSUB: "lamp","1" ...
      digitalWrite(LED_BUILTIN, LOW);                        // ...swith lamp on
    }
  }
  if ((millis() - start) > 10000) {
    start = millis();
    sendCommand("AT+CPSI?", "+CPSI:", 2000);                 // print network connection every 10 seconds
    sendCommand("AT+SMPUB=\"temp\",4,0,0", ">", 3000);       // send temp
    Modemboard.print(dht.readTemperature(),1);               // do the measurement here so allow some time for the > prompt
    delay(10);                                               // wait a bit for the modem to finish sending temp
    sendCommand("AT+SMPUB=\"humi\",4,0,0", ">", 3000);       // send hum
    Modemboard.print(dht.readHumidity(),1);                  // do the measurement here so allow some time for the > prompt
  }
}