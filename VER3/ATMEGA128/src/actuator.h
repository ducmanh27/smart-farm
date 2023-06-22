/**
 * @file actuator.h
 * @author HAIDT191811
 * @brief This library include function to control biến tần with RS485 or 4 opto-GPIOs
 * @date 2023-06-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "header.h"

#define DEVICE_ADDRESS 0x01

void FORWARD_RUNNING();
void STOP();
void REVERSE_RUNNING();
void SET_FREQUENCY();
void MAXFREQUENCY_FORWARD_RUNNING();
void MAXFREQUENCY_REVERSE_RUNNING();
void optoPinInit();
void run_multispeed(int n);
#endif