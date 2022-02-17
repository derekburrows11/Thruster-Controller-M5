
#pragma once

#include "BMS_Comms_Msgs.h"

void BMSRequestData();
bool BMSOnRxdPkt(uint8_t* pData, size_t length);   // return 1 if no errors

extern struct BMSData_Info         BMS_Info;
extern struct BMSData_VoltageCells BMS_VoltageCells;
extern struct BMSData_Version      BMS_Version;
