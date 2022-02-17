
#include "config.h"
#include "imu.h"
#include <Vector.h>
#include <Thruster_DataLink.h>
//#include "_Main Controller M5.h"

MPU6886& imu = M5.IMU;

float pitch = 0;
float roll  = 0;
float yaw   = 0;
float tempMPU6886;
Vector3f acc, gyro, gyroRad, ahrs;


void imu_init() {
  imu.Init();

  twoKi = 2.0;    // is 0 by default.  Just need to offset gyro reading to average 0
}

void imu_updateAHRS() {
  imu.getGyroData(&gyro.x, &gyro.y, &gyro.z);
  imu.getAccelData(&acc.x, &acc.y, &acc.z);
  gyroRad = gyro * DEG_TO_RAD;
//  gyroRad = 0;
//  acc = 0;
  // Default sample rate is 25Hz
  MahonyAHRSupdateIMU(gyroRad.x, gyroRad.y, gyroRad.z, acc.x, acc.y, acc.z, &pitch, &roll, &yaw);     // Attitude-Heading Reference System: pitch, roll, yaw
  
  ctrl.throttle = (roll - 0.0) * 100.0/60;

}


void imu_updatePage() {
  imu.getTempData(&tempMPU6886);
  
#define X_UNITS 240
  lcd.setCursor(0, 30);
  lcd.printf(" %6.2f  %6.2f  %6.2f      ", gyro.x, gyro.y, gyro.z);
  lcd.cursor_x = X_UNITS;
  lcd.println("d°/s");

  lcd.setCursor(0, 60);
  lcd.printf(" %6.2f  %6.2f  %6.2f      ", acc.x, acc.y, acc.z);
  lcd.cursor_x = X_UNITS;
  lcd.println("G");

  lcd.setCursor(0, 90);
  lcd.printf(" %6.2f  %6.2f  %6.2f      ", pitch, roll, yaw);
  lcd.cursor_x = X_UNITS;
  lcd.println("d°");

  lcd.setTextColor(ORANGE, BLACK);
  lcd.setCursor(0, 120);
  lcd.printf("Temperature:  %.1f °  \xB0 a \260 C \n", tempMPU6886);   // deg = ° (\xB0 or \176(dec) or \260(oct)) - not printing with font 4, some fonts only have 96 characters

}