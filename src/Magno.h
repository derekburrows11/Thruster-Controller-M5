
#if USE_MAGNO

#ifndef _Magno_h_
#define _Magno_h_

#include <MLX90393.h>
#include <Vector.h>

const float pi = asin(1) * 2;
//const float pi = M_PI;
const float rad2deg = 180 / pi;


struct magnoData {
	float mag;
	float magLog;
	float magInv;
	float magInvSqrt;		// field = 1/x^2 -> x = 1/sqrt(field)
	Vector3f vtFieldTx;
	Vector3f vtField;
	Vector3f vtFieldNorm;
};



// extend MLX90393 class for more internal access
class Magno : public MLX90393 {
  
public:
  void Init();
  bool Check();
  float GetThrottle();
  float GetSensorTemp();
  void mlxReadConfig();
  float GetMagnitude() { return mag; }
  

  bool FindMagLocation();     // returns true if location is recognised

//----------------------------

  float GetAngleDeg(float x, float y);
  void printRaw();
  void printuT();
  void printMagmT(float mag);

  uint8_t readData_Start();
  uint8_t readData_Get(MLX90393::txyz& data);

public:
  int msDone;

protected:
  int msTime, msTime_Prev;
  float mag;      // uT
  float magxy;    // uT
  float fThrottle;  // %
};


#endif  // _Magno_h_
#endif  // USE_MAGNO
