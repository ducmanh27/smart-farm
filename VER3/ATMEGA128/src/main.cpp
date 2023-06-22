#include <Arduino.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include "header.h"
#include "sensor.h"
#include "actuator.h"
#include "handleOperator.h"

SoftwareSerial UART1(PIN_PD2, PIN_PD3); // RX, TX. UART1 for MAX485
SoftwareSerial UART2(PIN_PE2, PIN_PE3); // RX, TX. UART2 for ESP07
SoftwareSerial UART3(PIN_PE4, PIN_PE5); // RX, TX. UART3 for MHZ16
Timer t;
int SensorId = 0;
char macAddress[18] = {};

/*************************** FUNCTION TIMER LOOP ****************************************/
void KeepAlive(void *context);
void sendDataEnv(void *context);
void sendDataAct(void *context);

/*************************************** SETUP ***************************************/
void setup()
{
  pinMode(LEDPIN, OUTPUT);
  Serial.begin(9600);
  UART1.begin(9600);
  UART2.begin(9600);
  UART3.begin(9600);
  optoPinInit();
  sensorInit();
  registration();
  // setup for timer loop
  t.every(T_SENSOR_DATA, sendDataEnv, (void *)0);
  t.every(T_ACTUATOR_DATA, sendDataAct, (void *)0);
  t.every(T_KEEP_ALIVE, KeepAlive, (void *)0);
}

/***************************************__LOOP__***************************************/
void loop()
{
  t.update();
  // Processing data received from ESP07
  if (Serial.available())
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    DeserializationError err = deserializeJson(doc, Serial.readString());

    if (err == DeserializationError::Ok)
    {
      ledDebug();                              // signal for received data
      switch (switchOperator(doc["operator"])) // swich operator
      {
      case RESISTER_ACK:
        handleResisterAck(&doc);
        break;
      case RESISTER:
        handleResister(&doc);
        break;
      case KEEP_ALIVE:
        handleKeepAlive(&doc);
        break;
      case SENSOR_DATA:
        handleSensorData(&doc);
        break;
      case ACTUATOR_DATA:
        handleActuatorData(&doc);
        break;
      case SET_POINT:
        handleSetPoint(&doc);
        break;
      case SET_POINT_ACK:
        handleSetPointAck(&doc);
        break;
      default:

        break;
      }
    }
    else
    {
      while (Serial.available() > 0)
        Serial.read(); // Flush all bytes in the "link" serial port buffer
    }
  }
}

/**
 * @brief make msg that included measured values and send to esp07
 *{
  "operator": "sensorData",
  "status": 0,
  "id ": 3,
  "info": {
    "co2": 400,
    "temp": 60.52,
    "hum": 29.32,
    "time": 1655396252
  }
 */
void sendDataEnv(void *context)
{

  if (!UART2.available() && SensorId)
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "sensorData";
    doc["status"] = 0;
    doc["id"] = SensorId;
    doc["info"]["time"] = now();
    doc["info"]["co2"] = readMHZ16();
    doc["info"]["temp"] = readTemp();
    doc["info"]["hum"] = readHum();

    serializeJson(doc, UART2);
  }
}
/**
 * @brief make keepAlive msg values and send to esp07
 *
 * {
  "operator":"keepAlive",
  "info": {
    "id":1,
    "time":1655396252,
  }
}
 */
void KeepAlive(void *context)
{
  if (!UART2.available() && SensorId)
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "keepAlive";
    doc["info"]["id"] = SensorId;
    doc["info"]["time"] = now();
    serializeJson(doc, UART2);
  }
};
/**
 * @brief make msg that included actuator values and send to esp07
 *{
  "operator": "actuatorData",
  "status": 0,
  "id ": 3,
  "info": {
    "state": 1,
    "speed": "15%",
    "time": 1655396252
  }
}
 */
void sendDataAct(void *context)
{
  if (!UART2.available() && SensorId)
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "actuatorData";
    doc["status"] = 0;
    doc["id"] = SensorId;
    doc["info"]["time"] = now();
    doc["info"]["state"] = 0;
    doc["info"]["speed"] = 0;
    serializeJson(doc, UART2);
  }
};
