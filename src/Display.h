#pragma once

// #include <M5Core2.h>
// extern M5Buttons& buttons;


enum DISPLAY_PAGE_t {
  PAGE_MAIN = 0,
  PAGE_DRIVEFB,
  PAGE_CTRLPOWER,
  PAGE_TIMING,
  PAGE_IMU,
  PAGE_MEMORY,

  PAGE_LAST,

  PAGE_SUB0 = 0,
  PAGE_SUB1 = 20,
  PAGE_SUB2 = PAGE_SUB1 * 2,
  PAGE_SUB3 = PAGE_SUB1 * 3,
};


void display_init();
void display_update();
void display_message(const char* str);
void display_messagesClear();


void page_change(int pg);
void page_changeNext();
void page_changePrev();
void page_changeSubNext();
void page_changeSubPrev();

void page_update();

void page_setup_statusBar();
void page_updateText_statusBar();

// Pages ////////
////////////////
void page_setupMain();
void page_updateMain();
void page_updateMain_Text();

void page_setupDriveFB();
void page_updateDriveFB();
void page_updateDriveFB_Text();

void page_setupCtrlPower();
void page_updateCtrlPower();
void page_updateCtrlPower_Text();

void page_setupCtrlPowerS1();
void page_updateCtrlPowerS1();
void page_updateCtrlPowerS1_Text();

void page_setupTiming();
void page_updateTiming();
void page_updateTiming_Text();

void page_setupIMU();
void page_updateIMU();
void page_updateIMU_Text();

void page_setupMem();
void page_updateMem();
void page_updateMem_Text();


