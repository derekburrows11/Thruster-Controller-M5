#pragma once


#include "config.h"


uint8_t axp_GetAdcSamplingRate();       //Adc rate can be read from 0x84
float axp_GetBatChargeCurrent();        // axp.GetBatChargeCurrent() is wrong as Read12Bit(0x7A) * 0.5, is 13 bit not 12 bit
float axp_GetBatDischargeCurrent();     // axp.GetBatDischargeCurrent() is not defined


void power_init();
void power_checkInterrupts();
int power_getSeconds();
void power_incSeconds();

int power_idleCheck();
int power_idleTimeOut();
void power_idleReset();
bool power_idleShutdown();


void power_lightSleep();
void power_deepSleep();
void power_shutdown();

void power_standby();
void power_wakeup();
void power_restart();




void motor_vibe_pulse();
void motor_vibe(int msDuration);
void motor_vibe_check(int ms);
void motor_vibe_off();

