// Config for Thruster Controller on M5stack-core2
// M5 Core2 with 320x240 LCD
// touchscreen, SD, 390mAh battery

#include <M5Core2.h>

//#define Use_SerialBT 1    // defined in _Main
#define UseRadio 1
#define USE_BLE 0   // no BLE files in controller yet - for comms to Li ion battery BMS
#define USE_MAGNO 1


extern M5Display& lcd;
extern M5Touch& touch;
extern M5Buttons& buttons;
extern AXP192& axp;


