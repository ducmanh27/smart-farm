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

SoftwareSerial UART1(PIN_PD2, PIN_PD3); // RX, TX. UART1 for MAX485
#define DEVICE_ADDRESS 0x01
unsigned char d1[30];

void FORWARD_RUNNING();
void STOP();
void REVERSE_RUNNING();
void SET_FREQUENCY();
void MAXFREQUENCY_FORWARD_RUNNING();
void MAXFREQUENCY_REVERSE_RUNNING();
void pinInit();

void pinInit()
{
  for (int i = 41; i >= 38; i--)
  {
    digitalWrite(i, LOW);
    pinMode(i, OUTPUT);
  }
}

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
  unsigned int i = 0;
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

  for (int i = 0; i < 8; i++)
  {
    UART1.write(d1[i]);
  }
}
void FORWARD_RUNNING()
{
  send485(DEVICE_ADDRESS, 0x00, 0x0001);
}
void REVERSE_RUNNING()
{
  send485(DEVICE_ADDRESS, 0x00, 0x0002);
}
void STOP()
{
  send485(DEVICE_ADDRESS, 0x00, 0x0005); // 01 06 20 00 00 05
}
void SET_FREQUENCY()
{
  send485(DEVICE_ADDRESS, 0x01, 0x0032); // 32 H =50 D
}
void MAXFREQUENCY_FORWARD_RUNNING()
{
  send485(DEVICE_ADDRESS, 0x05, 0x0032);
}
void MAXFREQUENCY_REVERSE_RUNNING()
{
  send485(DEVICE_ADDRESS, 0x06, 0x0032);
}

int run_multispeed(int n) // n = [0,15]
{
  int binaryNum[4] = {0, 0, 0, 0};
  int i = 0;
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
  return n;
}