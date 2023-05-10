#include <Arduino.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <Timer.h>
#include <string.h>

/*include library for sensors*/
#include "RTClib.h"
RTC_DS3231 rtc;

// led for debuging
#define LEDPIN PIN_PB7
int SensorId = 0;
#define NumOfOperator 5
const char Operators[NumOfOperator][20] = {"RegistrationAck", "Registration", "keepAlive", "sensorData", "actuatorData"};

Timer t;
SoftwareSerial UART1(PIN_PD2, PIN_PD3); // RX, TX. UART1 for MAX485
SoftwareSerial UART2(PIN_PE2, PIN_PE3); // RX, TX. UART2 for ESP07
SoftwareSerial UART3(PIN_PE4, PIN_PE5); // RX, TX. UART3 for MHZ16

void ledDebug();
void Registration();
void Processing();
void KeepAlive(void *context);
void ReadSHT21(void *context);
void ReadMHZ16(void *context);
void ReadMotor(void *context);
void sendData(void *context);

void setup()
{
  // put your setup code here, to run once:
  pinMode(LEDPIN, OUTPUT);
  Serial.begin(9600);
  UART1.begin(9600);
  UART2.begin(9600);
  UART3.begin(9600);

  /*RTC DS3231 setup; when lost power, time will have new setup*/
  while (!rtc.begin())
    ;
  if (rtc.lostPower())
    rtc.adjust(DateTime(2022, 10, 07, 14, 56, 0));

  /* Registration*/
  Registration();

  /* setup for timer*/
  t.every(5000, ReadMHZ16, (void *)0);
  t.every(5000, ReadSHT21, (void *)0);
  t.every(5000, ReadMotor, (void *)0);
  t.every(1800000, KeepAlive, (void *)0);
  t.every(5000, sendData, (void *)0);
}

void loop()
{
  t.update();
  Processing();
}

/*************************************************************************************
***************************************FUNCTION***************************************
**************************************************************************************/
// blink led
void ledDebug()
{
  digitalWrite(PIN_PB7, 1);
  delay(100);
  digitalWrite(PIN_PB7, 0);
  delay(100);
  digitalWrite(PIN_PB7, 1);
  delay(100);
  digitalWrite(PIN_PB7, 0);
  delay(100);
}

/* Receive Jsondata from ESP*/
void Processing()
{
  if (Serial.available())
  {
    StaticJsonDocument<512> doc;
    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, Serial.readString());

    if (err == DeserializationError::Ok)
    {
      int Ocase = -1;
      for (int i = 0; i < NumOfOperator; i++)
      {
        if (doc["operator"] == Operators[i])
        {
          // ledDebug();
          Ocase = i;
        }
      }

      switch (Ocase)
      {
      case 0:
        SensorId = doc["info"]["id"];
        serializeJson(doc, UART2);
        break;
      case 1:

        break;
      case 2:
        // ledDebug();
        break;
      case 3:
        ledDebug();
        break;
      case 4:
        // ledDebug();
        break;

      default:
        // ledDebug();
        break;
      }
    }
    else
    {
      // Flush all bytes in the "link" serial port buffer
      while (Serial.available() > 0)
        Serial.read();
    }
  }
}

/* Registration MAC Address: EA:DB:84:AA:4E:CE */
void Registration()
{
  if (!UART2.available())
  {
    StaticJsonDocument<512> doc;
    doc["operator"] = "Registration";
    doc["info"]["macAddress"] = "EA:DB:84:AA:4E:CE";
    serializeJson(doc, UART2);
  }
}

/*make json string and send to ESP07*/
void sendData(void *context)
{
  if (!UART2.available())
  {
    StaticJsonDocument<512> doc;
    doc["operator"] = "sensorData";
    doc["id"] = SensorId;

    doc["info"]["time"] = rtc.now().unixtime() - 7 * 3600;
    doc["info"]["co2"] = random(400, 2000);
    doc["info"]["temperature"] = random(10, 40);
    doc["info"]["humidity"] = random(20, 90);

    doc["status"] = 0;
    serializeJson(doc, UART2);
  }
}

void KeepAlive(void *context)
{
  if (!UART2.available())
  {
    StaticJsonDocument<512> doc;
    doc["operator"] = "keepAlive";
    doc["info"]["time"] = rtc.now().unixtime() - 7 * 3600;
    doc["info"]["id"] = SensorId;
    doc["status"] = 0;
    serializeJson(doc, UART2);
  }
};
void ReadSHT21(void *context){};
void ReadMHZ16(void *context){};
void ReadMotor(void *context){};