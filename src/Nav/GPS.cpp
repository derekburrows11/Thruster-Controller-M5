/*
    Description: Use COM.GPS Module to get the coordinate data and time of the current location.
    Please install library before compiling:  
    TinyGPSPlus: https://github.com/mikalhart/TinyGPSPlus

    Setup to work with M5 Stack GPS AT6558 + MAX2659 module
*/
//#include <Arduino.h>

#include <M5Core2.h>
#include <TinyGPS++.h>    // there is also an older?? <TinyGPSPlus.h> library
#include "GPS.h"

// A sample NMEA stream.
const char *gpsStream =
  "$GPRMC,045103.000,A,3014.1984,N,09749.2872,W,0.67,161.46,030913,,,A*7C\r\n";
// "$GPRMC, Time, A?, latitude ddmm.mmmm, N/S, longitude dddmm.mmmm, E/W, 0.67?, 161.46?, ddmmyy,,, A*7C?
// 72 bytes for $GPRMC sentence = 75ms @ 9600 baud, plus other sentences

// The TinyGPS++ object
TinyGPSPlus gps;

// Home
double  lat_target = -33.10720;   // approx  1m
double long_target = 151.64475;   // approx  1m * cos(lat) = 0.84m

unsigned long msLastRx;


void displayCourse() {
  M5Display& lcd = M5.Lcd;

  double lat_now = gps.location.lat();
  double long_now = gps.location.lng();
  double dist = gps.distanceBetween(lat_now, long_now, lat_target, long_target);    // m
  double dir  = gps.courseTo(lat_now, long_now, lat_target, long_target);


  lcd.println();
  lcd.print("Home    ");
  lcd.print(dir, 0);
  lcd.print(" deg   ");
  lcd.print(dist, 1);
  lcd.println("m               ");

  lcd.print(F("Horz Prec:  ")); 
  if (gps.hdop.isValid()) {
    Lcd.print(gps.hdop.hdop(), 1);    // Horizontal Diminution of Precision
    Lcd.print("m      ");
  }
  else
    Lcd.print(F("---"));

}

void displayInfo()
{
  M5Display& lcd = M5.Lcd;
  lcd.setTextWrap(0);
  int xValues = 140;
  char strBlank4[] = "    ";

  int Tsec = millis() / 1000;
  if (Tsec & 1)
    M5.Lcd.setTextColor(GREEN, BLACK);
  else
    M5.Lcd.setTextColor(YELLOW, BLACK);

  lcd.setTextFont(4);
  M5.Lcd.setCursor(0, 10);

  M5.Lcd.print(F("Time:  "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.hour());
    M5.Lcd.print(F(":"));
    if (gps.time.minute() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.minute());
    M5.Lcd.print(F(":"));
    if (gps.time.second() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.second());
    M5.Lcd.print(F("."));
    if (gps.time.centisecond() < 10) M5.Lcd.print(F("0"));
    M5.Lcd.print(gps.time.centisecond());
    lcd.print(strBlank4);
  }
  else
      M5.Lcd.print(F("---"));
  Lcd.println();




  M5.Lcd.print(F("Latitude:    ")); 
  lcd.cursor_x = xValues;
  if (gps.location.isValid()) {
    M5.Lcd.print(gps.location.lat(), 5);
    lcd.print(strBlank4);
  }
  else
    M5.Lcd.print(F("---"));
  M5.Lcd.println();

  M5.Lcd.print(F("Longitude:    ")); 
  lcd.cursor_x = xValues;
  if (gps.location.isValid()) {
    M5.Lcd.print(gps.location.lng(), 5);   
    lcd.print(strBlank4);
  }
  else
    M5.Lcd.print(F("---"));
  M5.Lcd.println();

  M5.Lcd.print(F("Altitude:    ")); 
  lcd.cursor_x = xValues;
  if (gps.altitude.isValid()) {
    M5.Lcd.print(gps.altitude.meters());
    Lcd.print("m      ");
  }
  else
    M5.Lcd.print(F("---"));
  M5.Lcd.println();

  gps.location.age();


  M5.Lcd.print(F("Satellites:    "));
  lcd.cursor_x = xValues;
  if (gps.satellites.isValid()) {
    M5.Lcd.print(gps.satellites.value());
    lcd.print(strBlank4);
  }
  else
    M5.Lcd.print(F("---"));
  M5.Lcd.println();

  Lcd.print(F("Speed:    ")); 
//  lcd.cursor_x = xValues;
  if (gps.speed.isValid()) {
    Lcd.print(gps.speed.kmph(), 1);
    Lcd.print(" kph @ ");
    if (gps.course.isValid()) {
      lcd.print(gps.course.deg(), 0);
      lcd.print("       ");
      
      gps.cardinal(gps.course.deg());
    }
  }
  else
    Lcd.print(F("---"));
  Lcd.println();




#if 0
  M5.Lcd.print(F("Date:  "));
  if (gps.date.isValid())
  {
    M5.Lcd.print(gps.date.month());
    M5.Lcd.print(F("/"));
    M5.Lcd.print(gps.date.day());
    M5.Lcd.print(F("/"));
    M5.Lcd.print(gps.date.year());
    lcd.print(strBlank4);
  }
  else
  {
    M5.Lcd.print(F("---"));
  }
  M5.Lcd.println();
#endif

  displayCourse();

}


void gps_init()
{
//  Serial2.begin(9600, SERIAL_8N1, 13, 14);    // for Core ??
  Serial2.begin(9600, SERIAL_8N1, G33, G32);    // for Core2 Port A.  Default Rx queue length is 256
    // don't need to send anything to GPS module, it just reports continuously
}

void gps_read()     // checks for Rx characters over serial
{
  while (Serial2.available() > 0) {     // Default Rx queue length is 256
    int val = Serial2.read();
    gps.encode(val);
    msLastRx = millis();
  }
}
