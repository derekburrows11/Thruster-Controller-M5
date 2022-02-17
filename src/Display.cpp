
#include "config.h"
#include "_Main Controller M5.h"
#include <Utils.h>    // for periodic timing functions
#include "Power.h"
#include "User Input.h"
#include "imu.h"

//#include <Thruster_Config.h>
#include <Thruster_DataLink.h>


#include "GFX_Elements.h"
#include "Display.h"

//#include <drive/axp/axp20x.h>
#include "axp20x.h"

class AXP20X_Class axp20x;

#define STATUSBAR_BG TFT_NAVY
#define STATUSBAR_HEIGHT 26
#define PAGE_Y_TOP  30


extern PeriodicTrigger trigFast;       // 50Hz trigger
extern PeriodicTrigger trigLCD;        // 25Hz screen update & for IMU AHRS calculation
extern PeriodicTrigger trigLCDText;   // 5Hz screen text update
extern PeriodicTrigger trigSec;      // 1Hz trigger


//DISPLAY_PAGE_t pageCurr = PAGE_MAIN;
int pageCurr = PAGE_MAIN;
int pageCurrSub = 0;
int pageSubs[PAGE_LAST];

int updateTextCycle = 0;    // staged text updating
int updateTextMax = 4;      // number of graphics updates for one text update



// Defines the buttons. Colors in format {bg, text, outline}
ButtonColors on_clrs =  {  RED, WHITE, WHITE};
ButtonColors off_clrs = {BLACK, WHITE, WHITE};
/*
Button tl(0, 0, 0, 0, false ,"tl - IMU",     off_clrs, on_clrs, TL_DATUM);
Button bl(0, 0, 0, 0, false, "bottom-left",  off_clrs, on_clrs, BL_DATUM);
Button tr(0, 0, 0, 0, false, "Shutdown",     off_clrs, on_clrs, TR_DATUM);
Button br(0, 0, 0, 0, false, "bottom-right", off_clrs, on_clrs, BR_DATUM);
*/

// Need to set protected M5Buttons::_finger[0,1].button to null so Button::fingerUp() doesn't get called when changing screen pages
class M5ButtonsExt : M5Buttons {    // only created to access protected _finger[2].button.
public:
  void clearFingerButton(Button* btn) {
    if (_finger[0].button == btn) _finger[0].button = nullptr;
    if (_finger[1].button == btn) _finger[1].button = nullptr;
  }
  void clearFingerButtons() {
    _finger[0].button = nullptr;
    _finger[1].button = nullptr;
  }
};


const int GfxElem_Max = 20;
int GfxElem_Num = 0;
GFX_ELEM GfxElem[GfxElem_Max];
int msgNum = 0;
int msgInitRow = 180;
int xCol1, xCol2, xCol3, xCol4;


void display_init() {
  log_i("axp20x begin result %d", axp20x.begin(Wire1, AXP192_SLAVE_ADDRESS));   // AXP192 library uses Wire1

  pageSubs[PAGE_MAIN] = 0;
  pageSubs[PAGE_CTRLPOWER] = 1;

  page_change(PAGE_MAIN);

}

void display_showJpeg() {
  uint8_t arjpeg[100];
  lcd.drawJpg(arjpeg, 100);
//  lcd.drawJpgFile();

}

void display_update() {
  page_update();
}

void display_message(const char* str) {
  if (msgNum++ == 0)
    lcd.setCursor(0, msgInitRow);
//  lcd.setTextFont(0);   // default for buttons is ??
  lcd.setTextColor(TFT_YELLOW);
  lcd.println(str);
}
void display_messagesClear() {
  if (msgNum != 0)
    lcd.fillRect(0, msgInitRow, TFT_WIDTH, TFT_HEIGHT - msgInitRow, BLACK);
  msgNum = 0;
}


void page_changeNext() {
  if (++pageCurr >= PAGE_LAST)
    pageCurr = 0;
  pageCurrSub = 0;
  page_change(pageCurr);
}
void page_changePrev() {
  if (--pageCurr < 0)
    pageCurr = PAGE_LAST - 1;
  pageCurrSub = 0;
  page_change(pageCurr);
}
void page_changeSubNext() {
  if (++pageCurrSub > pageSubs[pageCurr])
    pageCurrSub = pageSubs[pageCurr];
  if (pageSubs[pageCurr] != 0)
    page_change(pageCurr);
}
void page_changeSubPrev() {
  if (--pageCurrSub < 0)
    pageCurrSub = 0;
  if (pageSubs[pageCurr] != 0)
    page_change(pageCurr);
}


void PrintButtonInstances() {
  for (int i = 0; i < Button::instances.size(); ++i) {
    Button* bt = Button::instances[i];
    Serial.println(bt->getName());
  }
}

void page_change(int pg) {
  pageCurr = pg;
  Serial.printf("Changing to page: %d \n", pageCurr);
  Serial.printf("Num button instances: %d \n", Button::instances.size());
//  PrintButtonInstances();

  memory_logStats();

// delete button instances from 4 and above.  0-3 are background & BtnA/B/C

  for (int i = Button::instances.size()-1; i >= 4; --i) {
    Button* bt = Button::instances[i];
    //  bt->off.bg = bt->off.outline = bt->off.text = NODRAW;   // this will stop drawing on 'fingerUp', but deleted button will still be called
    ((M5ButtonsExt*)&M5.Buttons)->clearFingerButton(bt);   // Need to set protected M5Buttons::_finger[0,1].button to null so Button::fingerUp() doesn't get called
    delete bt;
  }
//  for (auto button : Button::instances) {
//    delete button; }
  Serial.printf("Buttons after delete: %d \n", Button::instances.size());
//  PrintButtonInstances();

  GfxElem_Num = 0;    // remove GfxElem's
  msgNum = 0;

  lcd.fillScreen(BLACK);
  lcd.setTextWrap(0);

  switch (pageCurr + pageCurrSub*PAGE_SUB1) {
  case PAGE_MAIN:
    page_setupMain(); break;
  case PAGE_DRIVEFB:
    page_setupDriveFB(); break;
  case PAGE_CTRLPOWER:
    page_setupCtrlPower(); break;
   case PAGE_CTRLPOWER + PAGE_SUB1:
    page_setupCtrlPowerS1(); break;
 case PAGE_TIMING:
    page_setupTiming(); break;
  case PAGE_IMU:
    page_setupIMU(); break;
  case PAGE_MEMORY:
    page_setupMem(); break;
    break;
  default:
    ;
  }
  page_setup_statusBar();
  buttons.draw();   // resets hidden though.  Need to not draw hidden ones!

  Serial.printf("Buttons after setup: %d \n", Button::instances.size());
  PrintButtonInstances();
}

void page_update() {
  if (++updateTextCycle >= updateTextMax) {
    updateTextCycle = 0;
    page_updateText_statusBar();
  }
  switch (pageCurr + pageCurrSub*PAGE_SUB1) {
  case PAGE_MAIN:
    page_updateMain(); break;
  case PAGE_DRIVEFB:
    page_updateDriveFB(); break;
  case PAGE_CTRLPOWER:
    page_updateCtrlPower(); break;
  case PAGE_CTRLPOWER + PAGE_SUB1:
    page_updateCtrlPowerS1(); break;
  case PAGE_TIMING:
    page_updateTiming(); break;
  case PAGE_IMU:
    page_updateIMU(); break;
  case PAGE_MEMORY:
    page_updateMem(); break;
  }

// if any Gfx
  for (int i = 0; i < GfxElem_Num; i++)
    GfxElem[i].Update();

}


void page_setup_statusBar() {
  lcd.fillRect(0, 0, lcd.width(), STATUSBAR_HEIGHT, STATUSBAR_BG);
}
void page_updateText_statusBar() {
    float battVolt = m5.Axp.GetBatVoltage();  // ttgo->power->getBattVoltage();
  //  float battmVolt = battVolt * 1000;
//    int battPC = (battVolt - 3.2)*100 + 0.5; // = m5.Axp.GetBatState(); //  ttgo->power->getBattPercentage();
    int battPC = (battVolt - 3.6)*200 + 0.5; // = m5.Axp.GetBatState(); //  ttgo->power->getBattPercentage();
    if (battVolt > 4.0) battPC = (battVolt - 4.0)*100 + 80.5;

    lcd.setTextFont(4);        // font 3&5 are not set!
//    lcd.setTextSize(4);        // pixel multiplier
//    lcd.setFreeFont(&FreeSerif12pt7b);
    uint16_t clrFG = TFT_GREEN;
    if (M5.Axp.isCharging())
      clrFG = TFT_WHITE;
    else if (battPC <= 10)
      clrFG = TFT_RED;
    else if (battPC <= 20)
      clrFG = TFT_ORANGE;
    lcd.setTextColor(clrFG, STATUSBAR_BG);

//    lcd.setCursor(140, 0);
//    lcd.printf("%.2fv  %d%%  \n", battVolt, battPC);
    char str[20];
    dtostrf(battVolt, 5, 2, str);
    strcat(str, "  ");
    itoa(battPC, str + strlen(str), 10);
    strcat(str, "%");

    lcd.setTextDatum(TR_DATUM);
    lcd.drawString(str, lcd.width(), 0);

  if (pageCurr == PAGE_MAIN) {
    strcpy(str, "   ");
    itoa(power_idleTimeOut(), str + strlen(str), 10);
    lcd.setTextColor(TFT_CYAN, STATUSBAR_BG);
  //  lcd.drawString(str, 40, 0);
    lcd.setCursor(10, 0);
    lcd.printf("%i   ", power_idleTimeOut());
  }  
  lcd.setTextDatum(TL_DATUM);



/*  RTC_TimeTypeDef RTC_Wake;
  RTC_Wake.Hours   = 10;
  RTC_Wake.Minutes = 30;
  RTC_Wake.Seconds = 0;
*/
  RTC_TimeTypeDef RTC_Now;
  char timeStrbuff[20];
  M5.Rtc.GetTime(&RTC_Now);
  sprintf(timeStrbuff, "%02d:%02d:%02d  ", RTC_Now.Hours, RTC_Now.Minutes, RTC_Now.Seconds);
  lcd.setCursor(100, 0);
  lcd.setTextColor(TFT_ORANGE, STATUSBAR_BG);
  lcd.println(timeStrbuff);

  lcd.setCursor(70, 0);
  lcd.setTextColor(TFT_RED, STATUSBAR_BG);
  lcd.print(pageCurr);

}



//  Main    ////////
////////////////////
void page_setupMain() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  //lcd.setTextDatum(TR_DATUM);
 
  lcd.setCursor(0, PAGE_Y_TOP);
  lcd.println("Batt V");
  lcd.println("Batt I");
  lcd.println("Motor I");
  lcd.println("Mtr RPM");


  // Setup GfxElem's for bars and values
  int idx = -1;
  GfxElem[++idx].SetParam(&ctrl.throttle);
  GfxElem[  idx].Init(GFX_fBARH, TFT_YELLOW, TFT_DARKGREY, 100, 210, 200, 20);
  GfxElem[  idx].pxPerUnit = 2;
/*
  GfxElem[++idx].SetParam("Scan");
  GfxElem[  idx].Init(GFX_BUTTONLABEL, TFT_YELLOW, TFT_RED, 10, 30, 80, 40, 10);
  GfxElem[  idx].SetCB1(gfxCB_BLEscan);

  GfxElem[++idx].SetParam("Conn");
  GfxElem[  idx].Init(GFX_BUTTONLABEL, TFT_GREEN, TFT_LIGHTGREY, 150, 30, 70, 40, 10);
  GfxElem[  idx].SetCB1(gfxCB_BLEconnect);
*/
  GfxElem[++idx].InitLast();
  GfxElem_Num = idx;


}
void page_updateMain() {
    if (updateTextCycle == 0)
        page_updateMain_Text();
}
void page_updateMain_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.setTextDatum(TR_DATUM);
  int pxVal = lcd.drawFloat(ctrl.throttle, 1, 90, 210);
  lcd.fillRect(0, 210, 90 - pxVal, lcd.fontHeight(), TFT_BLACK);    // blank out to left or right justified

  const int xVals = 120;
  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  lcd.setCursor(xVals, PAGE_Y_TOP);
  lcd.printf("%.1f   \n", drive.voltageBattery);
  lcd.cursor_x = xVals;
  lcd.printf("%.2f   \n", drive.currentBattery);

  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.cursor_x = xVals;
  lcd.printf("%.1f     \n", drive.currentMotor);
  lcd.cursor_x = xVals;
  lcd.printf("%.0f   \n", drive.rpm);
  
}

//  Drive Feedback  ////////
////////////////////
void page_setupDriveFB() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  //lcd.setTextDatum(TR_DATUM);
 
  lcd.setCursor(0, PAGE_Y_TOP);
  lcd.println("MOSFET");
  lcd.println("Motor");
  lcd.println("Batt V");
  lcd.println("Batt I");
  lcd.println("Motor I");
  lcd.println("Mtr RPM");
  lcd.println("Duty Cyc");
//  lcd.println("Throttle");


  // Setup GfxElem's for bars and values
  int idx = -1;
  GfxElem[++idx].SetParam(&ctrl.throttle);
  GfxElem[  idx].Init(GFX_fBARH, TFT_YELLOW, TFT_DARKGREY, 100, 210, 200, 20);
  GfxElem[  idx].pxPerUnit = 2;

  GfxElem[++idx].InitLast();
  GfxElem_Num = idx;


}
void page_updateDriveFB() {
    if (updateTextCycle == 0)
        page_updateDriveFB_Text();
}
void page_updateDriveFB_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.setTextDatum(TR_DATUM);
  int pxVal = lcd.drawFloat(ctrl.throttle, 1, 90, 210);
  lcd.fillRect(0, 210, 90 - pxVal, lcd.fontHeight(), TFT_BLACK);    // blank out to left or right justified

  const int xVals = 120;
  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  lcd.setCursor(xVals, PAGE_Y_TOP);
  lcd.printf("%.1f   \n", drive.tempMosfet);
  lcd.cursor_x = xVals;
  lcd.printf("%.1f   \n", drive.tempMotor);
  lcd.cursor_x = xVals;
  lcd.printf("%.1f   \n", drive.voltageBattery);
  lcd.cursor_x = xVals;
  lcd.printf("%.2f   \n", drive.currentBattery);

  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.cursor_x = xVals;
  lcd.printf("%.1f     \n", drive.currentMotor);
  lcd.cursor_x = xVals;
  lcd.printf("%.0f   \n", drive.rpm);
  lcd.cursor_x = xVals;
  lcd.printf("%.1f   \n", drive.dutyCycle*100);   // show in %
  
}

//  Controller Power     ////////
/////////////////////////////////
void page_setupCtrlPower() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP);
  lcd.println("Controller Powers");

/*
  axp.StopCoulombcounter();
  axp.DisableCoulombcounter();
  axp.EnableCoulombcounter();
  axp.ClearCoulombcounter();
  axp.SetCoulombClear();
  axp.EnableCoulombcounter();
*/
//  axp.ClearCoulombcounter();
//  axp.SetCoulombClear();
//  axp.StopCoulombcounter();
//  axp20x.StopCoulombcounter();    // could be setting wrong value, should be 0xC0 not 0xB8
  
//  axp20x.SetAdcState(1);
  log_i("adc rate %d", axp_GetAdcSamplingRate());

}
void page_updateCtrlPower() {
  if (updateTextCycle == 0)
    page_updateCtrlPower_Text();
}
void page_updateCtrlPower_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP + lcd.fontHeight());

// axp.Read32bit() library function only requested 2 bytes, changed to request 4 bytes 7/9/2021
  uint32_t charge = axp.GetCoulombchargeData();         // (0xB0)  ttgo->power->getBattChargeCoulomb();
  uint32_t discharge = axp.GetCoulombdischargeData();   // (0xB4)  ttgo->power->getBattDischargeCoulomb();
  uint8_t rate = axp_GetAdcSamplingRate();             // ttgo->power->getAdcSamplingRate();

  //charge = axp20x.getBattChargeCoulomb();         // (0xB0)  ttgo->power->getBattChargeCoulomb();
  //discharge = axp20x.getBattDischargeCoulomb();   // (0xB4)  ttgo->power->getBattDischargeCoulomb();
// axp.Read32bit() library function only requested 2 bytes, changes to request 4 bytes 6/9/2021
  lcd.printf("chg  %.1f mAh   \n",  axp.GetBatCoulombInput());   //=65536*0.5/3600/25.0 = 0.3641mAh  ttgo->power->getBattChargeCoulomb());      // has explanation
  lcd.printf("dis    %.1f mAh   \n", axp.GetBatCoulombOut());    // ttgo->power->getBattDischargeCoulomb());

  lcd.setTextColor(TFT_CYAN, TFT_BLACK);

  int xCol  = 100;
  int xCol2 = 200;
  lcd.setCursor(0, PAGE_Y_TOP + 30 + 2*26);
  //lcd.cursor_x = xCol;

  lcd.printf("Vin");
  lcd.cursor_x = xCol;
  lcd.printf("%.3f   ",    axp.GetVinVoltage());
  lcd.cursor_x = xCol2;
  lcd.printf("%.1f    \n", axp.GetVinCurrent());

  lcd.printf("VBus");
  lcd.cursor_x = xCol;
  lcd.printf("%.3f   ",    axp.GetVBusVoltage());
  lcd.cursor_x = xCol2;
  lcd.printf("%.1f    \n", axp.GetVBusCurrent());

  lcd.printf("VBat");
  lcd.cursor_x = xCol;
  lcd.printf("%.3f   ",    axp.GetBatVoltage());
  lcd.cursor_x = xCol2;
  lcd.printf("%.1f    \n", axp.GetBatCurrent());

//  lcd.printf("Crg Dis   %.1f   ", axp_GetBatChargeCurrent());
//  lcd.cursor_x = xCol2;
//  lcd.printf("%.1f   \n", axp_GetBatDischargeCurrent());

  lcd.printf("VAps");
  lcd.cursor_x = xCol;
  lcd.printf("%.3f   ", axp.GetAPSVoltage());
  lcd.cursor_x = xCol2;
  lcd.printf("%.0fmW   \n", axp.GetBatPower());   // in mW, when supplying only

  lcd.printf("Chrg1");
  lcd.cursor_x = xCol;
  lcd.printf("0x%x    ", axp.Read8bit(0x33));
//  lcd.cursor_x = xCol2;
//  lcd.printf("%.0fmW   \n", axp20x.tar());   // in mW, may need to x2


/*
  lcd.printf("C D     ");
  lcd.print(axp.Read13Bit(0x7A), 2);    // should be 13 bit
  lcd.print("    ");
  lcd.print(axp.Read13Bit(0x7C), 2);    // should be 13 bit
  lcd.println("    ");
*/


  //  lcd.printf("BPow %.2f   \n", axp.GetBatPower());

  //  lcd.printf("Temp %.1f  \n", axp.GetTempInAXP192());       // ttgo->power->getTemp());
  //  lcd.printf("TS Temp %.1f  \n", axp.getTSTemp());          // ttgo->power->getTSTemp());


  xCol = 220;
  lcd.setCursor(xCol, PAGE_Y_TOP + 0);
  lcd.setTextColor((axp.isACIN() ? TFT_GREEN : TFT_RED), TFT_BLACK);
  lcd.println("ACIN");

  lcd.cursor_x = xCol;
  lcd.setTextColor((axp.isCharging() ? TFT_GREEN : TFT_RED), TFT_BLACK);
  lcd.println("Charge");

  lcd.cursor_x = xCol;
  lcd.setTextColor((axp.isVBUS() ? TFT_GREEN : TFT_RED), TFT_BLACK);
  lcd.println("VBUS");


/*
    ttgo->power->StopCoulombcounter();
    ttgo->power->ClearCoulombcounter();
    ttgo->power->EnableCoulombcounter();
    ttgo->power->DisableCoulombcounter();

    ttgo->power->getCoulombData();
    ttgo->power->getCoulombRegister();
    ttgo->power->setCoulombRegister();
*/

}

//  Controller Power S1    ////////
/////////////////////////////////
void page_setupCtrlPowerS1() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP);
  lcd.println("Powers S1");
}
void page_updateCtrlPowerS1() {
  if (updateTextCycle == 0)
    page_updateCtrlPowerS1_Text();
}
void page_updateCtrlPowerS1_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP + lcd.fontHeight());

}



void doTimingReset(Event& e) {
  trigFast.reset();
  trigLCD.reset();
  trigLCDText.reset();
  trigSec.reset();
  lcd.fillScreen(TFT_BLACK);
}
void page_printTrig(const char* desc, PeriodicTrigger& trig) {
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.cursor_x = 0;
  lcd.printf(desc);
  lcd.cursor_x = xCol1;
  lcd.printf("%d   ", trig.dtMin);

  lcd.setTextColor(TFT_RED, TFT_BLACK);
  lcd.cursor_x = xCol2;
  lcd.printf("%d   ", trig.dtMax);

  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.cursor_x = xCol3;
  lcd.printf("%d   \n", trig.dt);

// Show stage times
  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  for (int i = 1; i < 3; i++) {
    if (trig.msStage[i] < 0)
      break;
    lcd.cursor_x = 20 + (i-1) * 80;
    lcd.printf("%d   ", trig.msStage[i]);
  }
  lcd.println();
}


//  Timing     ////////
////////////////////
void page_setupTiming() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_RED, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP);
//  lcd.println("Timing");

  Button* butReset  = new Button(220 , 200, 80, 40, false, "Reset" , off_clrs, on_clrs, MC_DATUM);
  butReset->addHandler(doTimingReset, E_TAP | E_PRESSED);

}
void page_updateTiming() {
  if (updateTextCycle == 0)
    page_updateTiming_Text();
}
void page_updateTiming_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP + 0 * lcd.fontHeight());
  xCol1  = 80;
  xCol2 = 140;
  xCol3 = 240;
  page_printTrig("20ms", trigFast);
  page_printTrig("40ms", trigLCD);
  page_printTrig("200m", trigLCDText);
  page_printTrig("1s",   trigSec);


}


//  IMU    ////////
////////////////////
void page_setupIMU() {
  int16_t w = lcd.width();
  int16_t h = lcd.height();
  int16_t butW = 100;
  int16_t butH = 50;

  Button* butIMU   = new Button(    0       , h - butH, butW, butH, false, "IMU"      , off_clrs, on_clrs, MC_DATUM);
  Button* butSleep = new Button((w - butW)/2, h - butH, butW, butH, false, "Sleep"    , off_clrs, on_clrs, MC_DATUM);
  Button* butShut  = new Button((w - butW)  , h - butH, butW, butH, false, "Shutdown" , off_clrs, on_clrs, MC_DATUM);
  //Button& bl = *new Button(     0, hh + 5, hw - 5, qh - 5, false, "bottom-left" , off_clrs, on_clrs, BL_DATUM);
  //Button& br = *new Button(hw + 5, hh + 5, hw - 5, qh - 5, false, "bottom-right", off_clrs, on_clrs, BR_DATUM);
  butIMU->addHandler(doIMU, E_TAP);
  butSleep->addHandler(doSleep, E_TAP | E_PRESSED);
  butShut->addHandler(doShutdown, E_TAP);
  //br.addHandler(showPerformance, E_TAP);
  //br.repeatDelay = 1000;    // for E_PRESSING

  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP);
//  lcd.println("IMU");

}
void page_updateIMU() {
  if (updateTextCycle == 0)
    page_updateIMU_Text();
}
void page_updateIMU_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
//  lcd.setCursor(0, PAGE_Y_TOP);

  imu_updatePage();

}


//  Mem    ////////
////////////////////
void page_setupMem() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_RED, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP);

  lcd.printf("Heap Size: \n");
  lcd.printf("Free: \n");
  lcd.printf("Min Free: \n");

  lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
  lcd.printf("PSRAM Size: \n");
  lcd.printf("Free: \n");
  lcd.printf("Min Free: \n");
}
void page_updateMem() {
  if (updateTextCycle == 0)
    page_updateMem_Text();
}
void page_updateMem_Text() {
  lcd.setTextFont(4);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setCursor(0, PAGE_Y_TOP);

  int xCol = 160 + 14;
  lcd.cursor_x = xCol;
  lcd.println(ESP.getHeapSize());
  lcd.cursor_x = xCol;
  lcd.println(ESP.getFreeHeap());
  lcd.cursor_x = xCol;
  lcd.println(ESP.getMinFreeHeap());

  xCol = 160;
  lcd.cursor_x = xCol;
  lcd.println(ESP.getPsramSize());
  lcd.cursor_x = xCol;
  lcd.println(ESP.getFreePsram());
  lcd.cursor_x = xCol;
  lcd.println(ESP.getMinFreePsram());


}
