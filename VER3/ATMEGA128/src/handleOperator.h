/**
 * @file actuator.h
 * @author HAIDT191811
 * @brief This library include function to handle when messages were received. Each function serves one operator.
 * @date 2023-06-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef HANDLE_OPERATOR_H
#define HANDLE_OPERATOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "header.h"
#include "actuator.h"
#include "sensor.h"


void registration();
int switchOperator(String myOperator);
void handleResisterAck(StaticJsonDocument<SIZE_OF_JSON> *doc);
void handleResister(StaticJsonDocument<SIZE_OF_JSON> *doc);
void handleKeepAlive(StaticJsonDocument<SIZE_OF_JSON> *doc);
void handleSensorData(StaticJsonDocument<SIZE_OF_JSON> *doc);
void handleActuatorData(StaticJsonDocument<SIZE_OF_JSON> *doc);
void handleSetPoint(StaticJsonDocument<SIZE_OF_JSON> *doc);
void handleSetPointAck(StaticJsonDocument<SIZE_OF_JSON> *doc);
#endif