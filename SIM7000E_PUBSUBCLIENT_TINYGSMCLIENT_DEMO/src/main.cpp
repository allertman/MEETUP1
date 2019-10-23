#include <Arduino.h>
//
//  Meet-up 29 Oct 2019
//  This code shows how we can build an IoT device using TinyGSM library that provides Arduino Client
//  MQTT is done using the Arduino PubSubClient

#include "Adafruit_Sensor.h"                             // Include external Sensor library
#include "DHT.h"                                         // Include external Sensor library
#define DHTPIN 13                                        // Define to which GPIO pin we connect the DHT sensor to the ESP32
#define DHTTYPE 11                                       // Define DHT sensor type, blue is DHT11, white is DHT22
DHT dht(DHTPIN, DHTTYPE);                                // Define the sensor object

#define TINY_GSM_MODEM_SIM7000                           // Define which modem we use
#define DUMP_AT_COMMANDS                                 // uncomment if we like to see all AT commands between ESP32 and Modemboard
#define TINY_GSM_RX_BUFFER 512

#include <TinyGsmClient.h>                               // Include external TinyGSM library

const char apn[] = "DefaultAPN";                         // in some networks you need to be specific about what APN definition to use
const char user[] = "";                                  // leave empty when not used
const char pass[] = "";                                  // leave empty when not used

#include <HardwareSerial.h>
HardwareSerial SerialAT(2);                              // use the available HW serial (UART2) for sending AT commands
#include <StreamDebugger.h>                              // comment out if no debugging needed
StreamDebugger debugger(SerialAT, Serial);               // comment out if no debugging needed
TinyGsm modem(debugger);                                 // comment out if no debugging needed
//TinyGsm modem(SerialAT);                               // uncomment if no debugging needed
TinyGsmClient client(modem);                             // TinyGSM delivers a client for usage

#include <PubSubClient.h>                                // with TinyGSM we can use standard PubSubClient
PubSubClient mqtt(client);                               // Define MQTT client instance
long lastReconnectAttempt = 0;                           // some variable to track timing for connecting to MQTT broker
unsigned long lastSend = 0;                              // some variable to set time between sensor measurements

void callback(char* topic, byte* payload, unsigned int length) {  // define Callback funtion, executed when MQTT message arrives
  Serial.print("MQTT Message arrived [");
  Serial.print(topic);                                   // show message topic on the serial monitor
  Serial.print("] ");
  for (int i = 0; i < length; i++) {                     // show message content on the serial monitor
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String topicStr = topic;                               // convert to String so we can easily compare with desired topic name
  if (topicStr == "lamp"){                               // if message arrived in the desired topic, we analyse its content
    if ((char)payload[0] == '1') {                       // if content starts with 1 we switch built in LED on
      digitalWrite(LED_BUILTIN, LOW);                    // Turn the LED on
    } else {
      digitalWrite(LED_BUILTIN, HIGH);                   // Turn the LED off
    }
  }
}

//connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage, boolean cleanSession)
boolean mqttConnect() {
  if (!mqtt.connect("SIM7000E", "eyneiyga", "_sTKZQbfemKK")) {  // Specify MQTT connection details, ClientID, username and password, but there is more possible
    Serial.println(" fail");
    Serial.println(mqtt.state());
  return false;
  }
  return mqtt.connected();
}

void setup() {
  pinMode(LED_BUILTIN,OUTPUT);                           // initialise GPIO pint with built in LED as an output
  Serial.begin(115200);                                  // initialise serial monitor to baudrate of 115200 
  SerialAT.begin(115200,SERIAL_8N1, 16, 17, false);      // initialise second serial port to is baudrate 115200 and define pins 16(RX) and 17(TX)
  Serial.print("Initializing modem...");
  modem.restart();                                       // can call init() instead of restart()
  Serial.println("Modem started");
  modem.sendAT("+CNMP=38");                              // select 38 LTE network, 13 for GSM
  delay(2000);


  Serial.print("Waiting for network...");                // Register to Mobile network
  if (!modem.waitForNetwork()) {
    Serial.print(".");
  while (true);
  }
  Serial.println("Network connected OK");

  Serial.print("Connecting to ");                        // Connect to Data network
  Serial.print(apn);
  Serial.print("...");
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(".");
  while (true);
  }
  Serial.println("GPRS connected OK");


  mqtt.setServer("farmer.cloudmqtt.com", 16633);         // Set the MQTT Broker address and port
  Serial.print("Connecting to MQTT broker...");
  mqttConnect();
  while (!mqtt.connected()) {
    Serial.print(".");
    mqttConnect();
    delay(1000);
  }
  Serial.println("MQTT connected OK");
  mqtt.setCallback(callback);                            // define the call back funtion to use when a message arrives
  mqtt.subscribe("lamp");                                // subcsribe to receive messages from a specific topic
  //mqtt.subscribe("#");                                 // subcsribe to receive messages from all topics
  
  dht.begin();                                           // initialise DHT sensor
}

void loop() {                                            // this part of the code will continously loop
  if (mqtt.connected()) {                                // when mqtt is connected we run the mqtt.loop() from the PubSubClient
    mqtt.loop();
  } else {
      unsigned long t = millis();
      if (t - lastReconnectAttempt > 10000L) {           // If not connected try reconnecting every 10 seconds
        lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
  if (millis()-lastSend>5000) {                          // send values approx every 5 seconds
    float t = dht.readTemperature();                     // read temperature value from DHT sensor
    float h = dht.readHumidity();                        // read humidity value from DHT sensor
    char charVal[10];                                    // buffer for conversion to string
    dtostrf(t, 4, 1, charVal);                           // convert temp float to string
    mqtt.publish("temp", charVal);                       // publish temperature in temp topic
    dtostrf(h, 4, 1, charVal);                           // convert hum float to string
    mqtt.publish("humi", charVal );                      // publish humidity in humi topic
    lastSend = millis();
    modem.sendAT("+CPSI?");                              // just checking if we are still on LTE CATM1
  }
}