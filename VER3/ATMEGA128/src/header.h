/**
 * @file actuator.h
 * @author HAIDT191811
 * @brief This library has global variables in program
 * @date 2023-06-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef HEADER_H
#define HEADER_H
#include <Arduino.h>

#define T_ACTUATOR_DATA 15 * 1000
#define T_SENSOR_DATA 10 * 1000
#define T_KEEP_ALIVE (unsigned long)30 * 60 * 1000
#define LEDPIN PIN_PB7
#define SIZE_OF_JSON 256
#define NUM_OF_OPERATOR 7
const char operatorList[NUM_OF_OPERATOR][20] = {"registerAck",
                                                "register",
                                                "keepAlive",
                                                "sensorData",
                                                "actuatorData",
                                                "setPoint",
                                                "setPointAck"};
enum OPERATER
{
    RESISTER_ACK,
    RESISTER,
    KEEP_ALIVE,
    SENSOR_DATA,
    ACTUATOR_DATA,
    SET_POINT,
    SET_POINT_ACK
};
extern SoftwareSerial UART1; // RX, TX. UART1 for MAX485
extern SoftwareSerial UART2; // RX, TX. UART2 for ESP07
extern SoftwareSerial UART3; // RX, TX. UART3 for MHZ16
extern int SensorId;
extern char macAddress[18];
extern void ledDebug();

#endif