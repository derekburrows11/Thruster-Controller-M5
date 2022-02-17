
#include "config.h"
#include "Power.h"
#include "_Main Controller M5.h"


uint16_t idleTime = 0;          // seconds idle
uint16_t idleTime_Timeout;      // seconds
uint16_t idleTime_Timeout_Battery = 600;    // seconds
uint16_t idleTime_Timeout_Supply = 3600;    // seconds
uint16_t sTime;    // seconds since started
bool pwrLEDState;

int sleep_wasSleeping = 0;      // 0=no, 1=lightsleep, 2=deepsleep, 10=powerup



// Some AXP functions in AXP20X library but not in AXP192 library
// Need to modify AXP192 library to make Read_bit() functions public
// also axp.Read32bit() library function only requested 2 bytes, need to change to request 4 bytes (7/9/2021)
uint8_t axp_GetAdcSamplingRate() {    //Adc rate can be read from 0x84
// from TTGO Watch AXP20X_Class
//   _readByte(AXP202_ADC_SPEED, 1, &val);          // AXP202_ADC_SPEED = 0x84
//    return 25 * (int)pow(2, (val & 0xC0) >> 6);   // result in Hz
  uint8_t val = 25 << ((axp.Read8bit(0x84) & 0xC0) >> 6);   // axp.Read8bit(0x84);   // private function...
  return val;
}
float axp_GetBatChargeCurrent() {    // axp.GetBatChargeCurrent() is wrong as Read12Bit(0x7A) * 0.5, is 13 bit not 12 bit
    return axp.Read13Bit(0x7A) * 0.5;
}
float axp_GetBatDischargeCurrent() {    // axp.GetBatDischargeCurrent() is not defined
    return axp.Read13Bit(0x7C) * 0.5;
}



void power_init() {
    axp.SetCHGCurrent(AXP192::kCHG_190mA);      // default is 100mA
//    axp.SetCHGCurrent(AXP192::kCHG_100mA);      // default is 100mA
    

}

void power_incSeconds() {
    ++sTime;
    ++idleTime;
    pwrLEDState = !pwrLEDState;
    axp.SetLed(pwrLEDState);

    if (sTime % 30 == 5)
        log_v("Seconds: %d", sTime);
}

void power_idleReset() {
    idleTime = 0;
}
// Return idle time seconds count
int power_idleCheck() {
    power_incSeconds();
    axp.isACIN();
    axp.isVBUS();

    if (M5.Axp.isVBUS() || M5.Axp.isACIN())      // If VBUS (5V Bus) or 'ACin' (USB supply) connected - longer timeout
        idleTime_Timeout = idleTime_Timeout_Supply;
    else 
        idleTime_Timeout = idleTime_Timeout_Battery;
    if (idleTime > idleTime_Timeout) {
        power_idleShutdown();
        power_idleReset();
    }
    return power_idleTimeOut();
}
int power_idleTimeOut() {
  return idleTime_Timeout - idleTime;
}

bool power_idleShutdown() {
    Serial.println("power_idleShutdown - Shutting Down");
//    motor_vibe(20);        // vibe time doesn't work if delaying instead of polling!!
    delay(20);
//    motor_vibe_off();
    power_lightSleep();     // is awake when returning
//    power_deepSleep();
//    power_shutdown();

    return 1;
}


void power_lightSleep() {   // 2.5mA, 6days (with 80MHz) - from My-TTGO-Watch
    Serial.println("power_lightSleep - Going to light sleep");
//    adc_power_off();
//    adc_power_release();

    memory_logStats();
    log_i("go standby");
    delay(200);

//    adc_power_release();
//    esp_sleep_enable_touchpad_wakeup();
//    esp_sleep_get_touchpad_wakeup_status();

//    setCpuFrequencyMhz(80);
//    setCpuFrequencyMhz(10);


//    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);     // AXP202_INT = 35
    //esp_sleep_enable_ext1_wakeup(_BV(AXP202_INT), ESP_EXT1_WAKEUP_ALL_LOW);     // AXP202_INT = 35, or GPIO_SEL_35

//    gpio_wakeup_enable((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
//    gpio_wakeup_enable((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
//    esp_sleep_enable_gpio_wakeup();    // can wakeup from light sleep only, deep sleep wakes straight away (and resets).
//    M5.Axp.SetSleep();
//    M5.Axp.PowerOff();
//    M5.Axp.PrepareToSleep();
//    M5.Axp.DeepSleep();

    // set to wake on touch - interrupt CST_INT (39), low when pressed - see Touch.ispressed()
    gpio_wakeup_enable((gpio_num_t)CST_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();    // can wakeup from light sleep only, deep sleep wakes straight away (and resets).


    // below based on M5 light sleep
//    M5.Axp.LightSleep(SLEEP_SEC(10));       // light sleep for 10 seconds, then wake and shut down fully.  Can wake on touch straight away in light sleep.

    esp_sleep_source_t wakeup;
    int numTimerWakeups = 60;        // light sleep for 10 seconds 60 times, then shutdown
    M5.Axp.PrepareToSleep();
    do {
        esp_sleep_enable_timer_wakeup(SLEEP_SEC(10));
        esp_light_sleep_start();        // sleeps here
        wakeup = esp_sleep_get_wakeup_cause();
        if (wakeup != ESP_SLEEP_WAKEUP_TIMER)
            break;
        Serial.printf("Woken on timer: %d\n", numTimerWakeups);
        M5.Axp.SetLed(1);
        delay(50);
        M5.Axp.SetLed(0);
        if (numTimerWakeups-- <= 0)
            power_shutdown();       // doesn't return
    } while (1);

    M5.Axp.RestoreFromLightSleep();


    // check wakeup source, if timer then shutdown fully
//    esp_sleep_source_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup == ESP_SLEEP_WAKEUP_GPIO)        // GPIO from touch interrupt CST_INT
        Serial.println("Woken on GPIO pin");
    else if (wakeup == ESP_SLEEP_WAKEUP_TIMER)
        Serial.println("Woken on timer");
    else if (wakeup == ESP_SLEEP_WAKEUP_TOUCHPAD)   // for esp's touchpad pins
        Serial.println("Woken on touch");
    else
        Serial.printf("Woken on other code: %d\n", wakeup);


//    esp_light_sleep_start();    // returns after waking on interrupt.
    sleep_wasSleeping = 1;      // was light sleep
    // Nothing was set to respone to interrupt.  Needs to turn off with 4sec press, then on with 2sec

    power_wakeup();
//    ttgo->displayWakeup();
//    ttgo->openBL();
//    tft->setTextColor(TFT_RED);
//    tft->println("Waking...");

}


void power_deepSleep() {   // ??mA
    Serial.println("power_deepSleep - Going to deep sleep");
    delay(200);

//    adc_power_off();        // from TTGO, needed?
    setCpuFrequencyMhz(20);

    M5.Axp.DeepSleep();
//    esp_deep_sleep_start();     // returns??  Nothing set to wake it.  Needs to turn off with 4sec press, then on with 2sec
    sleep_wasSleeping = 2;      // was deep sleep, but doesn't return from deepsleep routine
}


void power_shutdown() {
    Serial.println("power_shutdown - shutting down!");
    delay(200);
    M5.shutdown();

//    esp_register_shutdown_handler();

/*    ttgo->power->setTimeOutShutdown();
    ttgo->power->setlongPressTime();
    ttgo->power->setShutdownTime();
*/
}


void power_wakeup() {
    Serial.println("power_wakeup - waking up");
//    setCpuFrequencyMhz(240);
    setCpuFrequencyMhz(40);
//    setCpuFrequencyMhz(10);

//    ttgo->startLvglTick();
    //ttgo->displayWakeup();
   // m5.setWakeupButton();
    
//    ttgo->rtc->syncToSystem();
//    lv_disp_trig_activity(NULL);
//    ttgo->bma->enableStepCountInterrupt();

//    float sleepTime = (millis()-lenergySleepStart) * 0.001;
//    Serial.printf("WAKING UP... Slept for %.2f seconds\n", sleepTime);

//    ttgo->bl->adjust(120);
//    m5.Axp.SetLcdVoltage(1000);
//    m5.Axp.SetLCDRSet(1);
//    M5.Axp.RestoreFromLightSleep();     // done already by light sleep
    M5.Axp.ScreenBreath(10);        // brightness - DCDC3

 
}

void power_restart() {
   ESP.restart();
}



int motor_vibe_stopms;
bool motor_vibe_on;
void motor_vibe_pulse() {
//    digitalWrite(TTGO_Watch_MotorOut, 1);   // vibrate motor on
//    digitalWrite(TTGO_Watch_MotorOut, 0);   // vibrate motor off
    m5.Axp.SetLDOVoltage(3, 3300);     // AXP_103, LDO3 vib pwm
    m5.Axp.SetLDOVoltage(3, 0);     // AXP_103 vib pwm
    m5.Axp.SetLDOEnable(3, 1);     // AXP_103 vib pwm
    m5.Axp.SetLDOEnable(3, 0);     // AXP_103 vib pwm
}
void motor_vibe(int msDuration) {
    if (msDuration > 10000) msDuration = 10000;
    else if (msDuration < 0) msDuration = 0;
    motor_vibe_stopms = millis() + msDuration;
//    digitalWrite(TTGO_Watch_MotorOut, 1);   // vibrate motor on
    motor_vibe_on = 1;
}
void motor_vibe_check(int ms) {
    if (!motor_vibe_on || (ms - motor_vibe_stopms < 0))
        return;
//    digitalWrite(TTGO_Watch_MotorOut, 0);   // vibrate motor off
    motor_vibe_on = 0;
}
void motor_vibe_off() {
 //   digitalWrite(TTGO_Watch_MotorOut, 0);   // vibrate motor off
    motor_vibe_on = 0;
}

