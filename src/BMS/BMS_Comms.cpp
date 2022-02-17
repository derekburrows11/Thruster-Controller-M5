// For BMS General Protocol V4 from www.lithiumbatterypcb.com

#include <Arduino.h>

#include "BLE_client.h"
#include "BMS_Comms.h"
#include "BMS_Comms_Msgs.h"
#include "Pkt2Msg.h"


// Read the value of the characteristic.
// defining as char only sends first 3 or 4 ....
uint8_t msgBMSinfo[]    = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};   // for basic info and status
uint8_t msgBMSvoltage[] = {0xDD, 0xA5, 0x04, 0x00, 0xFF, 0xFC, 0x77};   // for voltages
uint8_t msgBMSversion[] = {0xDD, 0xA5, 0x05, 0x00, 0xFF, 0xFB, 0x77};   // for BMS version
uint8_t msgBMScontrol[] = {0xDD, 0x5A, 0xE1, 0x02, 0x00, 0x03, 0xFF, 0x1D, 0x77};   // for MOS control



uint8_t BMSMsgData[40];
class Pkt2Msg BMSMsg;

struct BMSMsgHead           BMS_Head;
struct BMSMsgTail           BMS_Tail;
struct BMSData_Info         BMS_Info;
struct BMSData_VoltageCells BMS_VoltageCells;
struct BMSData_Version      BMS_Version;

union  MOSFETControlByte    BMS_Control;  // .shutdownChargeMOS, .shutdownDischargeMOS


int reqType = 0;
int reqSeq = 0;

void BMSRequestData() {
    // Set the characteristic's value to be the array of bytes that is actually a string.
//    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
//    if(pRemoteCharacteristic->canWrite()) {

  switch (reqType) {
    case 0:
      remoteCharacteristicWrite(msgBMSinfo, sizeof(msgBMSinfo));
      Serial.println("Requested: basic info and status");
      break;
    case 1:
      remoteCharacteristicWrite(msgBMSvoltage, sizeof(msgBMSvoltage));
      Serial.println("Requested: cell voltages");
      break;
    case 2:
      remoteCharacteristicWrite(msgBMSversion, sizeof(msgBMSversion));
      Serial.println("Requested: BMS version");
      break;
    case 10:
      msgBMScontrol[5] = BMS_Control.byteVal;
      msgBMScontrol[7] = 0x100-0xE1-0x02 - BMS_Control.byteVal;
      remoteCharacteristicWrite(msgBMScontrol, sizeof(msgBMScontrol));
      Serial.println("Sending MOS Control");
      break;
    default:
      reqType = 0;
  }
  if(++reqSeq >= 10)
    reqSeq = 0;
  reqType = 0;
  if(reqSeq == 0)
    reqType = 1;

}

void BMSReadData() {    // doesn't work to read direct - get notified instead
  // Read the value of the characteristic.
  std::string value = "Not read yet";
  if (remoteCharacteristicRead(value)) {
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  } else {
    Serial.println("Can't Read Characteristic");
  }
}

bool BMSOnRxdPkt(uint8_t* pData, size_t length) {   // return 1 if no errors
  BMSMsg.SetMsg(BMSMsgData, sizeof(BMSMsgData));   // do this on init
  BMSMsg.OnRxdPkt(pData, length);
  if (!BMSMsg.CheckNewMsg())
    return 1;

  Serial.println("Got new message:");
  if (!BMS_Head.GetData(BMSMsgData)) {
    Serial.println("Header Invalid");
    return 0;
  }
  if (!BMS_Tail.GetData(BMSMsgData + BMSMsg.GetMsgBytesRxd())) {
    Serial.println("Tail Invalid");
    return 0;
  }

  switch (BMS_Head.msgType) {
    case 3:
      BMS_Info.GetData(BMSMsgData);
      BMS_Info.serialPrint();
      break;
    case 4:
      BMS_VoltageCells.GetData(BMSMsgData);
      BMS_VoltageCells.serialPrint();
      break;
    case 5:
      BMS_Version.GetData(BMSMsgData);
      BMS_Version.serialPrint();
      break;
    case 0xE1:
      BMS_Version.GetData(BMSMsgData);
      if (BMS_Head.error)
        Serial.println("MOS control command Failed......");
      else
        Serial.println("MOS control command Success");
      break;
    default:
      Serial.println("Message Not Valid");
      return 0;
  }
  Serial.println();
  return 1;
}
