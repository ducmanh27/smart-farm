/*
    Before using modbus protocol, VFD should be configured 
    P00.00 -> 2
    P00.01 -> 2 run by communication
    P00.03 -> 50.00 (Hz) Max Frequency 
    P00.04 -> 50.00 (Hz)
    P00.05 -> 00.00 (Hz)
    P00.06 -> 8 Function: MODBUS communication setting. The frequency is set by MODBUS communication
    P00.11 ->10 (s) the time needed if the inverter speeds up from 0 Hz to the MAX-P00.03 - acceleration (ACC)
    P00.12 ->10 (s) the time needed if the inverter speeds down from MAX-P00.03 Hz to 0 - deceleration (DEC)
    P02.01 -> 7.5 kW Rated power of AM 1 0.1 ~ 3000.0 kW
    P02.04 -> ... (V) Rated voltage
    P02.05 -> ... (A) Rated current
    P04.01 -> 2.0 (%) Torque boost
    P04.09 -> 0.0  (%)
    P14.00 -> 1 Address Mobus Slave
    P14.01 -> 3 (9600BPS) communication baud ratio
    P14.02 -> 0 No check
*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <Timer.h>
#include <string.h>
#include <ArduinoJson.h>
#include "SHT2x.h"
#include <string.h>

static unsigned char Address = 0x01;
const int ID = 1;
unsigned char hexdata[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
unsigned char d1[30];
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Timer t;
SHT2x sht;
SoftwareSerial UART1(PIN_PD2, PIN_PD3);
SoftwareSerial UART2(PIN_PE2, PIN_PE3);
SoftwareSerial UART3(PIN_PE4, PIN_PE5);
RTC_DS3231 rtc;

void FORWARD_RUNNING();
void STOP();
void REVERSE_RUNNING();
void SET_FREQUENCY();
void MAXFREQUENCY_FORWARD_RUNNING();
void MAXFREQUENCY_REVERSE_RUNNING();
unsigned int crc_chk(unsigned char *data, unsigned char length)
{
  int j;
  unsigned int reg_crc = 0xFFFF;
  while (length--)
  {
    reg_crc ^= *data++;
    for (j = 0; j < 8; j++)
    {
      if (reg_crc & 0x01)
      { /* LSB(b0)=1 */
        reg_crc = (reg_crc >> 1) ^ 0xA001;
      }
      else
      {
        reg_crc = reg_crc >> 1;
      }
    }
  }
  return reg_crc;
}
void send485(unsigned char device, unsigned char adress_low, unsigned int value)
{
  unsigned char i = 0;
  unsigned int crc = 0;
  // Serial.println(adress,HEX);
  // Serial.println(value);
  for (i = 0; i < 8; i++)
  {
    switch (i) //[0x01 0x06 0x20 0x00 0x00 0x01 crc] 01 06 3 20 05669
    {
    case 0:
      d1[i] = device;
      break; // default of INVT: 0x01
    case 1:
      d1[i] = 0x06;
      break; // 06H (correspond to binary 0000 0110), write one word(Word) => Master write data to the inverter and one command can write one data other than multiple dates
    case 2:
      d1[i] = 0x20;
      break; // The address instruction of other function in MODBUS 20,21,30 or 50 0x14
    case 3:
      d1[i] = adress_low;
      break; //
    case 4:
      d1[i] = 0xff & (value >> 8);
      break;
    case 5:
      d1[i] = 0xff & value;
      break;
    case 6:
      crc = crc_chk(d1, 6);
      d1[i] = crc & 0xff;
      break;
    case 7:
      d1[i] = 0xff & (crc >> 8);
      break;
    }
  }

  for (char i = 0; i < 8; i++)
  {
    Serial.write(d1[i]);
  }
  delay(2000);
}
void FORWARD_RUNNING()
{
  send485(Address, 0x00, 0x0001);
}
void REVERSE_RUNNING()
{
  send485(Address, 0x00, 0x0002);
}
void STOP()
{
  send485(Address, 0x00, 0x0005); // 01 06 20 00 00 05
}
void SET_FREQUENCY()
{
  send485(Address, 0x01, 0x0032); // 32 H =50 D
}
void MAXFREQUENCY_FORWARD_RUNNING()
{
  send485(Address, 0x05, 0x0032);
}
void MAXFREQUENCY_REVERSE_RUNNING()
{
  send485(Address, 0x06, 0x0032);
}

int temperature = 0, humidity = 0;
int CO2 = 0;
unsigned long time1 = 0;
boolean status = 1;
int speed = 0; // percent
int run_multispeed(int n);
void readCO2(void *context);
void readTH(void *context);
void speed_up()
{
  if (speed >= 0 and speed < 15)
  {
    speed++;
    run_multispeed(speed);
  }
  else
  {
    speed = 0;
    run_multispeed(speed);
  }
    
  
}
void speed_down()
{
  if (speed > 0)
  {
    speed--;
    run_multispeed(speed);
  }
  else{
    speed = 15;
    run_multispeed(speed);
  }
  
}
void receiveData(String json);
void sendData(void *context);
void setup()
{
  Serial.begin(9600);
  UART1.begin(9600);
  UART2.begin(9600);
  UART3.begin(9600);
  sht.begin();
  pinMode(PIN_PB7, OUTPUT);
  pinMode(PIN_PE6, INPUT_PULLUP);
  pinMode(PIN_PE7, INPUT_PULLUP);
  pinMode(PIN_PA3, OUTPUT);
  pinMode(PIN_PA4, OUTPUT);
  pinMode(PIN_PA5, OUTPUT);
  pinMode(PIN_PA6, OUTPUT);
  run_multispeed(speed);
  attachInterrupt(PIN_PE6, speed_up, FALLING);
  attachInterrupt(PIN_PE7, speed_down, FALLING);
#ifndef ESP8266
  while (!UART2)
    ; // wait for serial port to connect. Needed for native USB
#endif

  if (!rtc.begin())//
  {
    UART2.println("Couldn't find RTC");
    UART2.flush();
    while (1)
      delay(10);
  }
  if (rtc.lostPower())
  {
    UART2.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  t.every(3000, readCO2, (void *)0);
  t.every(3000, readTH, (void *)0);
  t.every(15000, sendData, (void *)0);
}
String readString;
void loop()
{
  digitalWrite(PIN_PB7,1);
  while (Serial.available())
  {
    delay(2); // delay to allow byte to arrive in input buffer
    char c = Serial.read();
    readString += c;
  }

  if (readString.length() > 0)
  {
    receiveData(readString);
    readString = "";
  }
  t.update();
}
void readCO2(void *context)
{
  UART3.write(hexdata, 9);
  for (int i = 0; i < 9; i++)
  {
    if (UART3.available() > 0)
    {
      long hi, lo;
      int ch = UART3.read();
      if (i == 2)
        hi = ch;
      if (i == 3)
        lo = ch;
      if (i == 8)
      {
        CO2 = hi * 256 + lo;
      }
    }
  }
}
void readTH(void *context)
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
}
int run_multispeed(int n) // n = [0,15]
{
  int binaryNum[4] = {0, 0, 0, 0};
  int i = 0;
  speed = n;
  while (n > 0)
  {
    binaryNum[i] = n % 2;
    n = n / 2;
    i++;
  }
  int k = 0;
  for (int i = 41; i >= 38; i--)
  {
    digitalWrite(i, !binaryNum[k]);
    k++;
  }
  return speed;
}
void sendData(void *context)
{
  StaticJsonDocument<500> doc;

  doc["id"] = ID;
  doc["status"] = status;
  JsonObject data = doc.createNestedObject("data");
  data["time"] = rtc.now().unixtime() - 7 * 3600;
  data["temerature"] = temperature;
  data["humidity"] = humidity;
  data["co2"] = CO2;
  JsonObject motor = doc.createNestedObject("motor");
  motor["status"] = speed == NULL ? "OFF" : "ON";
  motor["speed"] = speed;
  serializeJson(doc, UART2);
}
// Before receive data from ESP07, you need config hardware in MCU to receive RX0 <-> TX_ESP07
void receiveData(String json)
{

  StaticJsonDocument<128> doc;

  DeserializationError error = deserializeJson(doc, json);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  int id = doc["id"];
  bool status = doc["status"];

  long data_time = doc["data"]["time"];

  const char *motor_status = doc["motor"]["status"];
  int motor_speed = doc["motor"]["speed"];
  if (doc["motor"]["status"] == "1")
  {
    SET_FREQUENCY();
    delay(500);
    FORWARD_RUNNING();
    digitalWrite(PIN_PB7, 1);
  }
  else if (doc["motor"]["status"] == "0")
  {
    STOP();
    digitalWrite(PIN_PB7, 0);
  }
}
/*
{
  "id": 1,
  "status": true,
  "data": {
    "time": 1680019050
  },
  "motor": {
    "status": "1",
    "speed": 1
  }
*/