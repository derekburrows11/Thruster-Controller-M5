
#pragma once

#include "config.h"

//#include <Arduino.h>
//#include <Adafruit_GFX.h>    // Core graphics library
//#include "lvgl/src/lv_misc/lv_area.h"    // can use instead of config.h - just for areas

//#include "lv_area.h"
//#include <lv_conf.h>
//#include "../lv_conf_internal.h"

#include <Vector.h>
typedef Vector2<int16_t> Vector2pnt;


// Handle FLASH based storage e.g. PROGMEM  -  code copied from 'TFT_eSPI.h' line 56
#ifdef __AVR__
#include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#else
#define PROGMEM 1
#endif



// Some ready-made 16-bit ('565') color settings:
/* Defined colors
#define	ST77XX_BLACK      0x0000
#define ST77XX_WHITE      0xFFFF
#define	ST77XX_RED        0xF800  R
#define	ST77XX_GREEN      0x07E0  G
#define	ST77XX_BLUE       0x001F  B
#define ST77XX_CYAN       0x07FF  G + B
#define ST77XX_MAGENTA    0xF81F  R + B 
#define ST77XX_YELLOW     0xFFE0  R + G
#define	ST77XX_ORANGE     0xFC00  R + G/2
*/
// More colors
/*
//#define ST77XX_ORANGE   0xFC00  // R   + G/2
#define	ST77XX_LIME       0x87E0  // R/2 + G	
#define	ST77XX_LGREEN     0x07F4  // G   + B3/4	
#define	ST77XX_LBLUE      0x041F  // G/2 + B
#define ST77XX_PURPLE     0x801F  // B   + R/2
#define ST77XX_PINK       0xF810  // B/2 + R
#define ST77XX_DGREY      0x4208
#define ST77XX_GREY       0x8410
#define ST77XX_LGREY      0xc618
*/
/*
// Default color definitions
#define TFT_BLACK       0x0000      //   0,   0,   0  
#define TFT_NAVY        0x000F      //   0,   0, 128  
#define TFT_DARKGREEN   0x03E0      //   0, 128,   0  
#define TFT_DARKCYAN    0x03EF      //   0, 128, 128  
#define TFT_MAROON      0x7800      // 128,   0,   0  
#define TFT_PURPLE      0x780F      // 128,   0, 128  
#define TFT_OLIVE       0x7BE0      // 128, 128,   0  
#define TFT_LIGHTGREY   0xD69A      // 211, 211, 211  
#define TFT_DARKGREY    0x7BEF      // 128, 128, 128  
#define TFT_BLUE        0x001F      //   0,   0, 255  
#define TFT_GREEN       0x07E0      //   0, 255,   0  
#define TFT_CYAN        0x07FF      //   0, 255, 255  
#define TFT_RED         0xF800      // 255,   0,   0  
#define TFT_MAGENTA     0xF81F      // 255,   0, 255  
#define TFT_YELLOW      0xFFE0      // 255, 255,   0  
#define TFT_WHITE       0xFFFF      // 255, 255, 255  
#define TFT_ORANGE      0xFDA0      // 255, 180,   0  
#define TFT_GREENYELLOW 0xB7E0      // 180, 255,   0  
#define TFT_PINK        0xFE19      // 255, 192, 203    Lighter pink, was 0xFC9F      
#define TFT_BROWN       0x9A60      // 150,  75,   0  
#define TFT_GOLD        0xFEA0      // 255, 215,   0  
#define TFT_SILVER      0xC618      // 192, 192, 192  
#define TFT_SKYBLUE     0x867D      // 135, 206, 235  
#define TFT_VIOLET      0x915C      // 180,  46, 226  

#define	TFT_LIME       0x87E0  		  // 128, 255,   0
#define	TFT_LGREEN     0x07F4  		  //   0, 255,  192	
#define TFT_DGREY      0x4208
*/


enum GFX_FORMAT_t { // Font, decimal places, R or L justify
    GFX_FONT1 = 0,
    GFX_FONT2 = 1,
    GFX_FONT3 = 2,
    GFX_FONT4 = 3,
    GFX_DP0 = 0,
    GFX_DP1 = 4,
    GFX_DP2 = 8,
    GFX_DP3 = 12,
    GFX_RJUSTIFY = 0,
    GFX_LJUSTIFY = 0x10,
};

#define GFX_FORMAT_DP        ((format >>2) & 3)
#define GFX_FORMAT_FONT      ((format & 3) + 1) 
#define GFX_FORMAT_LJUSTIFY  ((format & GFX_LJUSTIFY) != 0) 


enum GFX_ELEM_t {   //Bits:  Bar, Value, Text, Vertical, Integer
    GFX_END        = 0,
    GFX_END_SCREEN = 0,
    GFX_BAR    = 0x01,
    GFX_VALUE  = 0x02,
    GFX_TEXT   = 0x04,

    GFX_PROGMEM    = 0x08,    // default is in RAM
    GFX_FLOAT  = 0x08,        // default is uint16_t
    GFX_VERT   = 0x10,
    GFX_SIGNED = 0x20,

    GFX_BUTTON = 0x100,
    GFX_BUTTONLABEL = GFX_BUTTON | GFX_TEXT,

    GFX_fBARH  = GFX_BAR | GFX_FLOAT,
    GFX_fBARHS = GFX_BAR | GFX_FLOAT | GFX_SIGNED,
    GFX_iBARH  = GFX_BAR,
    GFX_fVALUE = GFX_VALUE | GFX_FLOAT,
    GFX_iVALUE = GFX_VALUE,
    GFX_TEXTPROGMEM = GFX_TEXT | GFX_PROGMEM,
};

enum GFX_ELEM_RESPONSE_t {
  NONE = 0,
  TURN_ON,
  TURN_OFF,
  RAMP_ON,
  RAMP_OFF,
};

struct GFX_Var {      // 6 bytes
  int iSizePrev;    // pixels for Bar, chars for Value
  union {
    int iValPrev;	// not used for Bar
    float fValPrev;	// not used for Bar
  };
};


struct GFX_AREA {
  uint16_t  x, y;
  uint16_t  w, h;
  uint16_t   rad;      // corner radius
  bool contains(uint16_t x, uint16_t y) {

  }
};


struct GFX_ELEM : public Zone {
public:
                                                                                                                     
// constant parameters
  GFX_ELEM_t GFX;
  uint16_t  clr, clrTouched;
 
//  uint16_t  x, y;
//  uint16_t  w, h;
  uint8_t   rad;      // corner radius

  uint8_t format;     //  [fontSize(2b) | decPlace(2b) | justify(1b)]
  const void* param;   // param* or text*
  float     pxPerUnit;  // for bar
  //  static Adafruit_GFX *_gfx;  // = &tft;  static doesn't effect size

// variables

public:
  GFX_Var var;    // include in GFX_ELEM for now
  bool touchedElem, touchedElemPrev;
  bool touchedScreen, touchedScreenPrev;   // should be members of a single 'touch' monitoring class - use members in GFX_TOUCH

protected:

  GFX_ELEM_RESPONSE_t elemVisResponse;  // Element current visual change response
  int elemVisState;     // Element current visual state - scalar for colour

public:
  void (*FuncClk)(GFX_ELEM* gfx);
  void (*FuncDClk)(GFX_ELEM* gfx);
  void (*FuncHold)(GFX_ELEM* gfx);

public:
  void Init();
  void Init(GFX_ELEM_t gfx, uint16_t clrFg, uint16_t clrBk, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r = 0);
  // Set as last marker in a list
  void InitLast();
  // Set Value Parameter
  void SetParam(const void* _param) {param = _param; }
  // Button Click Callback
  void SetCB1(void (*func)(GFX_ELEM* gfx)) { FuncClk = func; }
  // Button Double Click Callback
  void SetCB2(void (*func)(GFX_ELEM* gfx)) { FuncDClk = func; }
  // Button Holddown Callback
  void SetCB3(void (*func)(GFX_ELEM *gfx)) { FuncHold = func; }

  void Update();

 
  int CheckPress();
  bool IsGfx() const { return GFX & (GFX_BAR | GFX_BUTTON); }

protected:
  bool UpdateGfx();
  void UpdateButton();
  void UpdateBar();
  void UpdateBarSigned();
  void UpdateValue();
  void UpdateStr(); // const PString& str doesn't work for SAMD M0
  void UpdateText() const;            // const PString& str doesn't work for SAMD M0

}; // struct GFX_ELEM
