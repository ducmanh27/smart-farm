#include "sensor.h"
SHT2x sht;
RTC_DS3231 rtc;
// hex data for MHZ16 read data ad calibration
unsigned char hexReadCo2[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
uint8_t hexZeroPoint[9] = {0xff, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};

void sensorInit()
{
    // SHT21 begin
    sht.begin();
    // RTC DS3231 setup; when lost power, time will have new setup
    rtc.begin();
    // while (!rtc.begin())
    //   ;
    rtc.adjust(DateTime(2023, 6, 22, 3, 0, 0));
};

float readTemp()
{

    if (sht.isConnected())
    {
        sht.read();
        return sht.getTemperature();
    }
    else
    {
        sht.reset();
        return -1;
    }
};

float readHum()
{
    if (sht.isConnected())
    {
        sht.read();
        return sht.getHumidity();
    }
    else
    {
        sht.reset();
        return -1;
    }
};

int readMHZ16()
{
    int hi = 0, lo = 0;

    UART3.write(hexReadCo2, 9);

    for (int i = 0; i < 9; i++)
    {
        if (UART3.available() > 0)
        {
            int ch = UART3.read();
            if (i == 2)
                hi = (int)ch;
            if (i == 3)
                lo = (int)ch;
            if (i == 8)
                return hi * 256 + lo;
        }
    }
    return -1;
};

void co2CalibrateZeroPoint()
{
    if (UART3.available() > 0)
    {
        UART3.write(hexZeroPoint, 9);
    }
}
unsigned long now()
{
    return rtc.now().unixtime() - 7 * 3600;
};