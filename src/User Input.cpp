
//#include "config.h"

#include "User Input.h"

#include "Display.h"
#include "Power.h"
#include "_Main Controller M5.h"
#include "imu.h"


// Defines gestures
Gesture swipeRight("swipe right", 160, DIR_RIGHT, 30, true);  // gesture index 0
Gesture swipeDown("swipe down", 120, DIR_DOWN, 30, true);     // gesture index 1
Gesture swipeLeft("swipe left", 160, DIR_LEFT, 30, true);     // gesture index 2
Gesture swipeUp("swipe up", 120, DIR_UP, 30, true);           // gesture index 3

enum GESTUREE {
  SWIPE_RIGHT,
  SWIPE_DOWN,
  SWIPE_LEFT,
  SWIPE_UP,
};

void userInput_init() {
  buttons_init();

}

void buttons_init() {
  buttons.addHandler(onGesture, E_GESTURE);
  buttons.addHandler(onDblTap, E_DBLTAP);
  buttons.addHandler(onEvent, E_ALL - E_MOVE);
//  buttons.addHandler(onEvent, E_ALL);
}



void onGesture(Event& e) {
  // Gestures and Buttons have an instanceIndex() that starts at zero
  // so by defining the gestures in the right order I can use that as
  // the input for M5.Lcd.setRotation.
  uint8_t gestIdx = e.gesture->instanceIndex();
//  if (new_rotation != M5.Lcd.rotation) {
//    lcd.clearDisplay();
//    lcd.setRotation(new_rotation);
//    doButtons();
//  }
  switch (gestIdx) {
  case SWIPE_UP:
    page_changeNext(); break;
  case SWIPE_DOWN:
    page_changePrev(); break;
  case SWIPE_LEFT:
    page_changeSubNext(); break;
  case SWIPE_RIGHT:
    page_changeSubPrev(); break;
  default:
    ;
  }

}

void onDblTap(Event& e) {
  // Just so we can type "b." instead of "e.button->"
  Button& b = *e.button;

  if (b != M5.background) {
    // Toggles the button color between black and blue
    b.off.bg = (b.off.bg == BLACK) ? BLUE : BLACK;
    b.draw();
  }
}

void showPerformance(Event& e) {
  performance_Print();
}

void onEvent(Event& e) {
  power_idleReset();

  Serial.printf("%-12s finger%d  %-18s (%3d, %3d) --> (%3d, %3d)   ",
                e.typeName(), e.finger, e.objName(), e.from.x, e.from.y,
                e.to.x, e.to.y);
  Serial.printf("( dir %d deg, dist %d, %d ms )\n", e.direction(),
                e.distance(), e.duration);
}

void doIMU(Event& e) {
  page_change(PAGE_IMU);
}

void doShutdown(Event& e) {
  power_shutdown();
}

void doSleep(Event& e) {
  power_lightSleep();
}

