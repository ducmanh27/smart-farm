#include <Arduino.h>
#include <string.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include "control.h"

// SHT21
#include "SHT2x.h"
SHT2x sht;
float temperature = 0;
float humidity = 0;

// DS3231 RTC
#include "RTClib.h"
RTC_DS3231 rtc;

// define timer to loop functions, time should be not equal because UART is overload in a time.
#define T_READ_SHT21 (unsigned long)120 * 1000
#define T_READ_MHZ16 30 * 1000
#define T_MHZ16_CALIBRATION (unsigned long)30 * 24 * 60 * 60 * 1000 // 1 month
#define T_READ_MOTOR 7 * 1000
#define T_SEND_DATA 30 * 1000
#define T_KEEP_ALIVE (unsigned long)30 * 60 * 1000
#define T_RE_REGISTER 4000

// led for debuging
#define LEDPIN PIN_PB7
#define SIZE_OF_JSON 256
#define NUM_OF_OPERATOR 5
const char operatorList[NUM_OF_OPERATOR][20] = {"registerAck", "register", "keepAlive", "sensorData", "actuatorData"};
int SensorId = 0;
char macAddress[18];
Timer t;

// hex data for MHZ16 read data ad calibration
unsigned char hexReadCo2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
uint8_t hexZeroPoint[9] = {0xff, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
int Co2 = 0;

// SoftwareSerial UART1(PIN_PD2, PIN_PD3); // RX, TX. UART1 for MAX485
SoftwareSerial UART2(PIN_PE2, PIN_PE3); // RX, TX. UART2 for ESP07
SoftwareSerial UART3(PIN_PE4, PIN_PE5); // RX, TX. UART3 for MHZ16

/*************************** FUNCTION PROTOTYPE ****************************************/
void ledDebug();
void Registration();
void KeepAlive(void *context);
void ReadSHT21(void *context);
void ReadMHZ16(void *context);
void co2CalibrateZeroPoint(void *context);
void sendDataEnv(void *context);
void sendDataAct();

/*************************************** SETUP ***************************************/
void setup()
{
  // put your setup code here, to run once:
  pinMode(LEDPIN, OUTPUT);
  pinMode(PIN_PA3, OUTPUT);
  pinMode(PIN_PA4, OUTPUT);
  pinMode(PIN_PA5, OUTPUT);
  pinMode(PIN_PA6, OUTPUT);
  Serial.begin(9600);
  UART1.begin(9600);
  UART2.begin(9600);
  UART3.begin(9600);

  // SHT21 begin
  sht.begin();

  // RTC DS3231 setup; when lost power, time will have new setup
  rtc.begin();
  // while (!rtc.begin())
  //   ;
  if (rtc.lostPower())
    rtc.adjust(DateTime(2022, 10, 07, 14, 56, 0));

  // register
  Registration();

  // setup for timer
  t.every(T_READ_MHZ16, ReadMHZ16, (void *)0);
  t.every(T_READ_SHT21, ReadSHT21, (void *)0);
  t.every(T_MHZ16_CALIBRATION, co2CalibrateZeroPoint, (void *)0);
  t.every(T_READ_MOTOR, ReadSHT21, (void *)0);
  t.every(T_KEEP_ALIVE, KeepAlive, (void *)0);
  t.every(T_SEND_DATA, sendDataEnv, (void *)0);
}

/***************************************__LOOP__***************************************/
void loop()
{
  t.update();
  // Processing data received from ESP07
  if (Serial.available())
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, Serial.readString());

    if (err == DeserializationError::Ok)
    {
      ledDebug();
      // select case
      int Ocase = -1;
      for (int i = 0; i < NUM_OF_OPERATOR; i++)
      {
        if (doc["operator"] == operatorList[i])
        {
          Ocase = i;
        }
      }

      // swich case
      switch (Ocase)
      {
      case 0:
        // "registerAck"
        if (doc["info"]["status"] == 1)
        {
          SensorId = doc["info"]["id"];
          strcpy(macAddress, doc["info"]["macAddress"]);
          serializeJson(doc, UART2);
        }
        else
        {
          Registration();
        }
        break;
      case 1:
        // "register"
        break;
      case 2:
        // "keepAlive"
        break;
      case 3:
        // "sensorData"
        break;
      case 4:
        // "actuatorData"
        sendDataAct();
        // run_multispeed(5);
        break;

      default:
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

/***************************************FUNCTION***************************************/

// register MAC Address: EA:DB:84:AA:4E:CE
void Registration()
{
  delay(T_RE_REGISTER);
  if (!UART2.available())
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "register";
    doc["info"]["macAddress"] = macAddress;
    doc["status"] = 0;
    serializeJson(doc, UART2);
  }
}

// make json string and send to ESP07
void sendDataEnv(void *context)
{
  if (!UART2.available() && SensorId)
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "sensorData";
    doc["id"] = SensorId;
    doc["info"]["time"] = rtc.now().unixtime() - 7 * 3600;
    doc["info"]["co2"] = Co2;
    doc["info"]["temperature"] = temperature;
    doc["info"]["humidity"] = humidity;
    doc["status"] = 0;
    serializeJson(doc, UART2);
  }
}

// make json to send keepAlive message
void KeepAlive(void *context)
{
  if (!UART2.available() && SensorId)
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "keepAlive";
    doc["id"] = SensorId;
    doc["info"]["time"] = rtc.now().unixtime() - 7 * 3600;
    doc["info"]["macAddress"] = macAddress;
    serializeJson(doc, UART2);
  }
};

void ReadSHT21(void *context)
{
  if (sht.isConnected())
  {
    sht.read();
    temperature = sht.getTemperature();
    humidity = sht.getHumidity();
  }
  else
  {
    sht.reset();
  }
};
void ReadMHZ16(void *context)
{
  UART3.write(hexReadCo2, 9);
  for (int i = 0; i < 9; i++)
  {
    if (UART3.available() > 0)
    {
      int hi, lo;
      int ch = UART3.read();
      if (i == 2)
        hi = (int)ch;
      if (i == 3)
        lo = (int)ch;
      if (i == 8)
      {
        Co2 = hi * 256 + lo;
      }
    }
  }
};

void co2CalibrateZeroPoint(void *context)
{
  if (UART3.available() > 0)
  {
    UART3.write(hexZeroPoint, 9);
  }
}
void sendDataAct()
{
  if (!UART2.available() && SensorId)
  {
    StaticJsonDocument<SIZE_OF_JSON> doc;
    doc["operator"] = "actuatorData ";
    doc["id"] = SensorId;
    doc["info"]["time"] = rtc.now().unixtime() - 7 * 3600;
    doc["info"]["state"] = 0;
    doc["info"]["speed"] = 0;
    doc["info"]["macAddress"] = macAddress;
    doc["status"] = 0;
    serializeJson(doc, UART2);
  }
};
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
