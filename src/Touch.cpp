
#include "Touch.h"


/*
// Declare the static members
Vector2pnt GFX_TOUCH::pnt;
Vector2pnt GFX_TOUCH::pntPrev;
Vector2pnt GFX_TOUCH::speedLP;
Vector2pnt GFX_TOUCH::speedGesture;
bool GFX_TOUCH::touchedScreen;
bool GFX_TOUCH::touchedScreenPrev;
bool GFX_TOUCH::touchedScreenPrev2;
int GFX_TOUCH::gesture;
void (*GFX_TOUCH::FuncGesture)();
*/

enum {
  GESTURE_NONE = 0,
  GESTURE_SWIPE_LEFT,
  GESTURE_SWIPE_RIGHT,
  GESTURE_SWIPE_UP,
  GESTURE_SWIPE_DOWN,
};

void GFX_TOUCH::CheckTouch() {    // function, call just once every scan before checking all gfx elements
  touchedScreenPrev2 = touchedScreenPrev;
  touchedScreenPrev = touchedScreen;
  pntPrev = pnt;
  pnt = 0;

//  touchedScreen = ttgo->getTouch(pnt.x, pnt.y);
  Point pt = M5.Touch.getPressPoint();
  touchedScreen = M5.Touch.ispressed();
  pnt.x = pt.x;
  pnt.y = pt.y;

  CheckGesture();
}

void GFX_TOUCH::CheckGesture() {    // function, call just once every scan before checking all gfx elements
  // track speed for swipe gestures - 20Hz updates
  Vector2pnt pntChange = pnt - pntPrev;
//  pntChange *= 100;
  if (touchedScreen)
    if (touchedScreenPrev) {    // also check for second scanned touch point
      if (touchedScreenPrev2) {
//        speedLP = speedLP * 0.9 + pntChange * 0.1;
        speedLP += (pntChange - speedLP) / 4;
      } else {      // first two points detected, speed directly from 2nd scanned touch point
        speedLP = pntChange;
      }
    } else {      // first touch, just set speed to zero
      speedLP = 0;
    }
  else
    if (touchedScreenPrev) {     // touch has just released, check speed and gesture
      speedGesture = speedLP;
      if (FindGestureType() && (FuncGesture != nullptr))
        FuncGesture();

    } else {    // 2 scans with no toucn
      speedLP = 0;
    }

}

bool GFX_TOUCH::FindGestureType() {
  gesture = GESTURE_NONE;
  if (speedGesture.magSq() < 400)
    return 0;
  Vector2pnt speedAbs(abs(speedGesture.x), abs(speedGesture.y));
  if (speedAbs.x > 2*speedAbs.y) {
    gesture = (speedGesture.x > 0) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
  } else if (speedAbs.y > 2*speedAbs.x) {
    gesture = (speedGesture.y > 0) ? GESTURE_SWIPE_DOWN : GESTURE_SWIPE_UP;
  } else
    return 0;
  return 1;
}
