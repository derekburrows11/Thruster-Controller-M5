#pragma once

#include "config.h"

#include <Vector.h>
typedef Vector2<int16_t> Vector2pnt;


class GFX_TOUCH {
public:

protected:
  // these members for tracking touch should be a different class with a single instance
  Vector2pnt pnt, pntPrev;   // touch point
public:
  Vector2pnt speedLP;        // Low Pass filter of speed.  Pixels/scan = Pixels/50ms
  Vector2pnt speedGesture;   // Set when gesture detected.  Normally when touch released

  bool touchedScreen, touchedScreenPrev, touchedScreenPrev2;
  int gesture;


  void (*FuncGesture)();

  // Touch Gesture detected Callback
  void SetCB_Gesture(void (*func)()) { FuncGesture = func; }

  void CheckTouch();  // static function, call just once on scan before checking all gfx elements
  void CheckGesture();
  bool FindGestureType();

};
