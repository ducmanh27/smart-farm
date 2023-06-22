/**
 * @file actuator.h
 * @author HAIDT191811
 * @brief This library include function to read value from sensors and rtc DS3231
 * @date 2023-06-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "SHT2x.h"
#include "RTClib.h"
#include "header.h"


void sensorInit();
float readTemp();
float readHum();
int readMHZ16();
void co2CalibrateZeroPoint();
unsigned long now();
#endif