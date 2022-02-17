
#include "config.h"
#if USE_MAGNO

#include "Magno.h"

#include <Helpers_Text.h>
#include <Thruster_Config.h>
#include <Thruster_DataLink.h>
#include <Vector.h>
#include <Vector3Txf.h>

const int magLow =   200;   // uT
const int magMin =   300;   // uT
const int magMax = 13000;   // uT


MLX90393::txyz    magField;    // units deg & uT.  1 Gauss = 100uT = 0.1mT. Earth's field approx 0.5 Gauss = 50uT. Magnet 50-500mT
MLX90393::txyzRaw magRaw;
MLX90393::txyz    magOffset = {0, -30, 0, -40}; 

struct magnoData magData;

float clamp(float val, float min, float max) {
  return val<min? min : (val>max? max : val);
}


uint8_t Magno::readData_Start() {
  uint8_t status1 = startMeasurement(X_FLAG | Y_FLAG | Z_FLAG | T_FLAG);
  msDone = millis() + convDelayMillis();
//  uint8_t I2C_address2 = I2C_address;
  return checkStatus(status1);
}
  
uint8_t Magno::readData_Get(MLX90393::txyz& data) {
  MLX90393::txyzRaw raw_txyz;
  uint8_t status2 = readMeasurement(X_FLAG | Y_FLAG | Z_FLAG | T_FLAG, raw_txyz);
  data = convertRaw(raw_txyz);
  return checkStatus(status2);
}


void Magno::Init() {

  Wire.begin();

  //Connect to sensor with I2C address jumpers set: A1 = 0, A0 = 0
  //Use DRDY pin connected to (not used) A3
  //Returns byte containing status bytes
  DEBUGSERIAL.fprintln("MLX90393 Begin...");
  byte statusbeg1 = begin();
//  byte statusbeg2 = begin();
  //Report status from configuration
  DEBUGSERIAL.fprint("Start status: 0x");
  DEBUGSERIAL.println(statusbeg1, HEX);
//  DEBUGSERIAL.print("  0x");
//  DEBUGSERIAL.println(statusbeg2, HEX);



//  DEBUGSERIAL.fprintln("Magno Init");
  uint8_t status1 = reset();
//  DEBUGSERIAL.fprintln("Magno 1");
  uint8_t status1b= checkStatus(reset());
//  DEBUGSERIAL.fprintln("Magno 2");
  uint8_t status2 = setGainSel(2);              // 0-7
//  DEBUGSERIAL.fprintln("Magno 3");
  uint8_t status3 = setResolution(1);     // 0-3
  uint8_t status4 = setOverSampling(0);         // 0-3, larger = longer
  uint8_t status5 = setDigitalFiltering(6);     // 0-7, larger = longer, 
  uint8_t status6 = setTemperatureCompensation(0);

  DEBUGSERIAL.fprintln("Statuses:");
  DEBUGSERIAL.println(status1, HEX);
  DEBUGSERIAL.println(status1b, HEX);
  DEBUGSERIAL.println(status2, HEX);
  DEBUGSERIAL.println(status3, HEX);
  DEBUGSERIAL.println(status4, HEX);
  DEBUGSERIAL.println(status5, HEX);
  DEBUGSERIAL.println(status6, HEX);

}


bool Magno::Check() {
  msTime = millis();
  
  uint8_t res;
  res = readData(magField);   //Read the values from the sensor
  magField.x -= magOffset.x;
  magField.y -= magOffset.y;
  magField.z -= magOffset.z;

 
/*  magField.x += 1;
  magField.y -= 2;
  magField.z += 3;
*/

  magData.vtField = Vector3f(magField.x, magField.y, magField.z);
  magxy = magData.vtField.magxy();    // uT
  mag = magData.vtField.mag();        // uT
  float OneOnMag = 100.0 / mag;
  magData.vtFieldNorm = magData.vtField * OneOnMag;   // magnitude 100
  
  magData.mag = mag;
  magData.magLog = 10 * log(mag);
  magData.magInv = clamp(10000 / mag, 0, 100);
  magData.magInvSqrt = sqrt(magData.magInv) * 10;   // clamped 0 to 100
  
  magData.vtFieldTx = magData.vtFieldNorm * (magData.magInvSqrt * 0.01);    // magnitude 0 to 100 (|.vtFieldNorm| = 100)



  float ang = GetAngleDeg(magxy, magField.z);

  fThrottle = (mag - magMin) * (100./(magMax - magMin));    // map mag 400-1500 to 0-100% throttle

  bool magOK = (mag > magLow);
  if (fThrottle < 0) fThrottle = 0;
  else if (fThrottle > 100) fThrottle = 100;
  if (!magOK) fThrottle = 0;    // if field is small, no throttle
  
//  DEBUGSERIAL.fprint("  Throttle ");
//  DEBUGSERIAL.println(fThrottle, 0);

  msTime_Prev = msTime;
  return true;
}

bool Magno::FindMagLocation() {
  Vector3f vtFieldTx = magData.vtFieldTx;      // magnitude 0 to 100

  // check if inside rotated box
  Vector3f vtCentre, vtFromCentre;
  Vector3f vtBoxBounds;
  Vector3f vtLBoxCoords, vtLBoxSize;
  Vector3Txf mxBoxTxf;
  
  vtFromCentre = vtFieldTx - vtCentre;
  bool bInsideWorld = vtFromCentre.isInsideBox(vtBoxBounds);
  if (bInsideWorld) {
    vtLBoxCoords = mxBoxTxf * vtFromCentre;
    bool bInsideLoc = vtLBoxCoords.isInsideBox(vtLBoxSize);


  }

  return 0;
}

float Magno::GetThrottle() {
  return fThrottle;
}
float Magno::GetSensorTemp() {
  return magField.t;
}

void Magno::mlxReadConfig() {
  uint8_t gs, rx, ry, rz, os, df, tc;
  getGainSel(gs);
  getResolution(rx, ry, rz);
  getOverSampling(os);       // default 3
  getDigitalFiltering(df);   // default 7
  getTemperatureCompensation(tc);

  DEBUGSERIAL.fprintln("Settings:");
  DEBUGSERIAL.fprint("Gain Select : "); DEBUGSERIAL.println(gs);
  DEBUGSERIAL.fprint("Resolution  : ");
  DEBUGSERIAL.print(rx); DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.print(ry); DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.println(rz);
  DEBUGSERIAL.fprint("Over Sample : "); DEBUGSERIAL.println(os);
  DEBUGSERIAL.fprint("Digital Filt: "); DEBUGSERIAL.println(df);
  DEBUGSERIAL.fprint("Temp Compens: "); DEBUGSERIAL.println(tc);
}


//----------------------------


float Magno::GetAngleDeg(float x, float y) {
  float fHeading;
  float absX = abs(x);
  float absY = abs(y);
  float iMagAbs = absX + absY;
  float iMag = sqrt(x*x + y*y);

  float fTan;
  if (iMag < 100) {
    fHeading = 999;
    return fHeading;
  }
  if (absX < absY)
    fTan = absX / absY;
  else
    fTan = absY / absX;

//  fHeading = atan(fTan);
  fHeading = atan2(x, y) * rad2deg;
  if (fHeading < 0)
    fHeading += 360;

  return fHeading;
}


void Magno::printRaw() {
  magRaw = raw_txyz;
  magRaw.x -= 0x8000;    // offset for 0 uT
  magRaw.y -= 0x8000;
  magRaw.z -= 0x8000;
  magRaw.t -= (46244 - 25*45.2);    // from data sheet
  float temp = magRaw.t / 45.2;
  
  DEBUGSERIAL.fprint("  [");
  DEBUGSERIAL.print((int)magRaw.x);
  DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.print((int)magRaw.y);
  DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.print((int)magRaw.z);
  DEBUGSERIAL.fprint("  temp ");
  DEBUGSERIAL.print(magRaw.t);
  DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.print(temp, 2);
  DEBUGSERIAL.fprintln("]");
}

void Magno::printuT() {
  DEBUGSERIAL.fprint(" mag uT ");
  DEBUGSERIAL.print(mag, 0);
  
  DEBUGSERIAL.fprint("  xyz ");
  DEBUGSERIAL.print(magField.x, 0);
  DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.print(magField.y, 0);
  DEBUGSERIAL.fprint("  ");
  DEBUGSERIAL.print(magField.z, 0);
  
//  DEBUGSERIAL.fprint(" temp ");
//  DEBUGSERIAL.print(magField.t);
}

void Magno::printMagmT(float mag) {
  DEBUGSERIAL.fprint("  Mag(mT) ");
  DEBUGSERIAL.print(mag * 0.001, 3);
}

#endif  // USE_MAGNO
