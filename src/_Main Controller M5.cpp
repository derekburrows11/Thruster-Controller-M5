

#define Use_SerialBT 1
// Other defines config.h
// #define UseRadio 0
// #define USE_BLE 0   // no BLE files in controller yet - for comms to Li ion battery BMS
// #define USE_MAGNO 1



// Can't effect log level with LOG_LOCAL_LEVEL  If set in platformio.ini:  build_flags = -DCORE_DEBUG_LEVEL
//#define LOG_LOCAL_LEVEL ESP_LOG_NONE
//#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
//#define LOG_LOCAL_LEVEL ESP_LOG_WARN
//#define LOG_LOCAL_LEVEL ESP_LOG_INFO
//#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

// redefine CORE_DEBUG_LEVEL works
//#define CORE_DEBUG_LEVEL  ARDUHAL_LOG_LEVEL_WARN
#define CORE_DEBUG_LEVEL  ARDUHAL_LOG_LEVEL_VERBOSE    // redefine CORE_DEBUG_LEVEL works to set level
//#define ARDUHAL_LOG_LEVEL  ARDUHAL_LOG_LEVEL_VERBOSE  // doesn't work if CORE_DEBUG_LEVEL not defined at least

//#define CONFIG_LOG_DEFAULT_LEVEL ESP_LOG_DEBUG
//#include "esp_log.h"   // is included elsewhere

#include "config.h"
#include <Utils.h>    // for periodic trigger
#include <MonitorLogColors.h>

#include "_Main Controller M5.h"
#include "User Input.h"
#include "Power.h"
#include "Display.h"
#include "imu.h"
#include "Magno.h"
#include "GPS.h"

#include <Thruster_Config.h>
#include <Thruster_DataLink.h>

#if UseRadio
  #include "CtrlLink_EspNow.h"
  CtrlLink_EspNowClass ctrlLink;
#endif


#include "EEPROM.h"   // use eeprom or maybe SPIFFS to store settings
#include "Preferences.h"   // prefered over EEPROM library.  Use EEPROM, Preferences or SPIFFS for files

// shortcuts for defined M5 objects
M5Display& lcd = M5.Lcd;
M5Touch& touch = M5.Touch;
M5Buttons& buttons = M5.Buttons;
AXP192& axp = M5.Axp;


//Define variables for data link
struct dataController ctrl;
struct dataDrive drive;

#if Use_SerialBT
  #include "BluetoothSerial.h"
  #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
    #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
  #endif
  BluetoothSerial SerialBT;
  char chBTData;
#endif

Magno magno;

// For performance measurement (Single tap on bottom-right button)
uint32_t loops = 0;
uint32_t msTimeStart;   // ms
int msTime;             // ms

PeriodicTrigger trigFast(20);       // 50Hz trigger
PeriodicTrigger trigLCD(40);        // 25Hz screen update & for IMU AHRS calculation
PeriodicTrigger trigLCDText(200);   // 5Hz screen text update
PeriodicTrigger trigSec(1000);      // 1Hz trigger

Preferences prefs;    // Non-volatile storage, Namespace and key max 15 characters


void setup() {
//  Serial.begin(115200);   // done by M5.begin()
//  M5.begin();     // M5.begin() does Serial.begin(115200).  This resets 'setDebugOutput' to NULL if does twice!
  M5.begin(true, true, true, false, kMBusModeInput);   // enable I2C for RTC, don't need to here anyway
//  Serial.setDebugOutput(true);    // Set log mesages to this Serial.  Is default, set to NULL if Serial.begin() done twice

  power_init();
  display_init();
  imu_init();   // takes about 0.5sec
  magno.Init();
  gps_init();

  performance_Init();
  userInput_init();   // if buttons handlers are added before any buttons, first button added gets background presses

#if Use_SerialBT
  if (SerialBT.begin("M5Controller"))  // Bluetooth device name, as slave
    log_i("BT Started OK");
  else
    log_e("BT Failed to Start");
#endif

#if UseRadio
  display_message("Init Wifi-Now");
  ctrlLink.Init();
/*
  display_message("Scanning WiFi...");
  ctrlLink.ScanWiFi();
  display_message("Checking Connections");
  ctrlLink.CheckForConnection();
  display_message("Connecting");
*/

  display_messagesClear();
//  page_change(PAGE_MAIN);

#endif


#if 0   // set time if required
  RTC_TimeTypeDef RTC_Set;
  RTC_Set.Hours   = 10;
  RTC_Set.Minutes = 25;
  RTC_Set.Seconds = 0;
  M5.Rtc.SetTime(&RTC_Set);
#endif

// include space before \ in "\033" to make escape sequece work for platformio 'miniterm' ?? doesn't fix miniterm
//#define ARDUHAL_LOG_COLOR(COLOR)  " \033[0;" COLOR "m"
//#define ARDUHAL_LOG_BOLD(COLOR)   " \033[1;" COLOR "m"
//#define ARDUHAL_LOG_RESET_COLOR   " \033[0m"


  if (!prefs.begin("Ctrl-wifi"))
    log_e("Can't open preferences: Ctrl-wifi");
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  if (ssid == "" || password == "") {
    Serial.println("No values saved for ssid or password.  Setting defaults.");
    ssid = "Telstra DerekAla";
    password = "mumanddad";
    prefs.putString("ssid", ssid);
    prefs.putString("password", ssid);
  } else {
    Serial.printf("Loaded wifi ssid: %s, pw: %s\n", ssid, password);
  }
  prefs.end();

  // get drive id's and passwords of most recently connected drives
  if (!prefs.begin("Ctrl-link", true))
    log_e("Can't open preferences: Ctrl-link");
  int numDrives = prefs.getInt("numDrives");
  String driveid = prefs.getString("drive001");
  password = prefs.getString("pw001", "");
  int16_t mostRecent[20];     // sorted list of most recent drive indexs connected
  size_t mostRecentAvai = prefs.getBytesLength("mostRecent");
  size_t mostRecentLen  = prefs.getBytes("mostRecent", mostRecent, sizeof(mostRecent));
  prefs.end();

// To completely erase and reformat the NVS memory used by Preferences:
// #include <nvs_flash.h>
//  nvs_flash_erase(); // erase the NVS partition and...
//  nvs_flash_init(); // initialize the NVS partition.


  Serial.println(LOG_CYAN "About to Setup EEPROM" LOG_RESET);
  Serial.println(BOLD_YELLOW "Next Line");

  log_e("Setting up EEPROM log_e");
  log_d("Setting up EEPROM log_d");

  log_i("EEPROM length: %d", EEPROM.length());
  EEPROM.begin(10600);    // can do 10,630 bytes, can't do 10,631 bytes
//  EEPROM.readFloat(1);
  log_i("EEPROM length: %d", EEPROM.length());

  EEPROM.end();
  log_i("EEPROM length: %d", EEPROM.length());

}

void loop() {
  loops++;
  msTime = millis();

  M5.update();    // does touch & butons update, and yield.  It can only read touch about every 15-20ms

  if (trigFast.checkTrigger(msTime)) {
//      dt = msFast.dt / 1000.0;
      // Get acceleration data
//        ttgo->bma->getAccel(acc);

    magno.Check();
    trigFast.done(1);
    gps_read();   // receives about 1 char/ms so may have ~20 chars to process.  Default buffer is 256 bytes
    trigFast.done(2);

    trigFast.doneTrigger();
  }

  if (trigLCD.checkTrigger(msTime)) {
    imu_updateAHRS();     // 1ms exec
    trigLCD.done(1);
    display_update();     // 34ms max
    trigLCD.done(2);

    #if !UseRadio
      ctrlLink.SendTx();
      trigLCD.done(3);
    #endif
    trigLCD.doneTrigger();
  }

  if (trigLCDText.checkTrigger(msTime)) {

    trigLCDText.doneTrigger();
  }

  if (trigSec.checkTrigger(msTime)) {
    power_idleCheck();
    trigSec.done(1);

  // config log level
  //  esp_log_level_set("*", ESP_LOG_INFO);
  //  ESP_LOGE(TAG, "1 second trigger - error");
  //  ESP_LOGD(TAG, "1 second trigger - debug");

  //  log_e("1 second trigger - log_e");
  //    log_d("1 second trigger - log_d");
  //    Serial.printf("1sec from serial\n");    // possibily doen't transmit if uart buffer full from log_ calls??

#if Use_SerialBT
    if (++chBTData < 'a') chBTData = 'a';
    if (chBTData > 'z') chBTData = 'a';
    SerialBT.write(chBTData);
    trigSec.done(2);
#endif

    trigSec.doneTrigger();
  }


#if UseRadio
  ctrlLink.CheckRx();     // if time since received packet from drive >1sec, set comms as failed.  Keep sending info while throttle not idle.
#endif

}


void performance_Init() {
  msTimeStart = millis();
}
void performance_Print() {
  Serial.printf("%d in %ld ms, average M5.update() took %.2f microseconds\n",
      loops, millis() - msTimeStart, (float)((millis() - msTimeStart) * 1000) / loops);
  msTimeStart = millis();
  loops = 0;
}


void memory_logStats() {
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Min Free heap: %d\n", ESP.getMinFreeHeap());

    Serial.printf("Free PSRAM heap: %d\n", ESP.getFreePsram());
    Serial.printf("uptime: %ld\n", millis() / 1000);

    log_i("Free heap: %d", ESP.getFreeHeap());
    log_i("Free PSRAM heap: %d", ESP.getFreePsram());
    log_i("uptime: %d", millis() / 1000 );

}
