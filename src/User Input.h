#pragma once

#include "config.h"

void userInput_init();
void buttons_init();

void onGesture(Event& e);
void onDblTap(Event& e);
void showPerformance(Event& e);
void onEvent(Event& e);
void doIMU(Event& e);
void doShutdown(Event& e);
void doSleep(Event& e);


