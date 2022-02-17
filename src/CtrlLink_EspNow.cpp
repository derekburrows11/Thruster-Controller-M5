#include "config.h"

#if UseRadio


#include "CtrlLink_EspNow.h"

#include <Helpers_Text.h>
#include <Thruster_Config.h>
#include <Thruster_DataLink.h>

//#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "NoPrint.h"
#define DEBUGSERIAL NoPrint

#define CHANNEL 1     // can be different for Tx & Rx
#define PRINTSCANRESULTS 1
#define DELETEBEFOREPAIR 0
#define TIMEOUTms_RX 1000

//void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
//void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

const char *SSIDDrive = "Thruster Drive";
const char *PWDrive = "Drive_1_Password";   // if required ?
const char *SSIDController = "Thruster Ctrl";

// Thruster Drive      [4C:11:AE:7B:C8:94] 94 as STA, 95 as AP ??
// Thruster Controller [24:6F:28:A2:6D:D8] D8 as STA, D9 as AP ??
static uint8_t macThrusterDrive[6] = {0x4C, 0x11, 0xAE, 0x7B, 0xC8, 0x94};  // Thruster Drive WROOM-32 board
//static uint8_t macThrusterCtrl[6]  = {0x24, 0x6F, 0x28, 0xA2, 0x6D, 0xD8};  // Controller WROOM-32 board


// define static member instances
bool CtrlLink_EspNowClass::bTxOK;
bool CtrlLink_EspNowClass::bRxNewData;
bool CtrlLink_EspNowClass::bRxNewDataReading;    // semaphore to Rx callback
bool CtrlLink_EspNowClass::bRxNewDataMissed;
int  CtrlLink_EspNowClass::iRxNewDataLen;
uint8_t CtrlLink_EspNowClass::buffRx[ESPNOW_MAX_RX_SIZE];



void CtrlLink_EspNowClass::Init() {
  Serial.println("Thruster Controller ESPNow Init");

// Not normally an AP
/*
  // For Access Point Mode
  if (!WiFi.mode(WIFI_MODE_AP))     // configure in Access Point mode
    Serial.println("WiFi AP Mode Failed");
  if (WiFi.softAP(SSIDController, PWDrive, CHANNEL, 0))    // PW need to be at least 8 chars if used
    Serial.println("AP Config Success.  Broadcasting with AP: " + String(SSIDController));
  else
    Serial.println("AP Config failed.");
  Serial.print("Controller AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());
*/


  // For Station Mode
  if (!WiFi.mode(WIFI_STA))     // configure in Station mode
    Serial.println("WiFi STA Mode Failed");
  Serial.print("Controller STA MAC: ");
  Serial.println(WiFi.macAddress());


// Init ESPNow
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK)
    Serial.println("ESPNow Init Success");
  else {
    Serial.println("ESPNow Init Failed");
    // Retry esp_now_init(), add a counter and then restart?
    ESP.restart();    // or Simply Restart
  }
  // clear data link structures
  memset(&ctrl,  0, sizeof(ctrl));
  memset(&drive, 0, sizeof(drive));
  bConnected = 0;
  bRxNewData = 0;

  // Once ESPNow is successfully Init, we will register for Send CB to get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);


// Below are not EspNow, just Esp wifi commands
//  esp_err_t esp_wifi_set_max_tx_power(int8_t power);    // unit is 0.25dBm, range is [40, 82] corresponding to 10dBm - 20.5dBm h

// setup Channel State Information (CSI) recieved callbacks to report packet RSSI
//esp_err_t  esp_wifi_set_csi_rx_cb(wifi_csi_cb_t cb, void *ctx);
  esp_wifi_set_csi_rx_cb(OnCsiRxd, NULL);
  // esp_wifi_set_csi_config(const wifi_csi_config_t *config);  // probably don't need to call
  esp_wifi_set_csi(true);
}

// callback when CSI data is recv
void CtrlLink_EspNowClass::OnCsiRxd(void *ctx, wifi_csi_info_t *data) {

}

void CtrlLink_EspNowClass::End() {
  esp_now_deinit();
}

bool CtrlLink_EspNowClass::CheckRx() {
  uint16_t msNow = millis();
  if (!bRxNewData) {
    if (msNow - msLastMsgRx > TIMEOUTms_RX) {
      RxTimedOut = 1;
      RxOK = 0;
      drive.SetRxTimedOut();
    }
    return 0;
  }

  // Should be a message for us now
  struct dataDriveToController fromDrive;
  uint8_t len = sizeof(fromDrive);
  packetsRxTotal++;
  RxOK = 0;     // confirm packet is OK first

  int numRx = GetRxData((uint8_t*)&fromDrive, len);
  if (numRx != len) {
    packetsRxError++;
    Serial.fprintln("Receive failed");
    return 0;
  }

  // check what sort of packet and from what drive
  if (!(fromDrive.id & DL_ID_DEST_CTRL))   // check ID is for control
    return 0;
  if (!(fromDrive.id & (DL_IDbit_SRC_DRIVE1 | DL_IDbit_SRC_DRIVE2)))    // check source is drive 1 or 2
    return 0;
  if ((fromDrive.id & DL_IDmask_TYPE) == DL_ID_TYPE_FAST) {   // check ID packet type
//    memcpy(&fromDrive, buf, sizeof(fromDrive));
    drive.GetData(fromDrive);
  }
  else
    return 0;

  packetsRxMe++;
  msLastMsgRx = msNow;
  RxTimedOut = 0;
  RxOK = 1;

  ctrl.packetsRx++;
  ctrl.rssi = 0;

  DEBUGSERIAL.print(msLastMsgRx);
  DEBUGSERIAL.fprint("ms  Rxd packet ");
  DEBUGSERIAL.println(packetsRxTotal);

  DEBUGSERIAL.print(msLastMsgRx - msLastMsgSent);
  DEBUGSERIAL.fprintln("ms  for send/Rx");

  DEBUGSERIAL.fprint("Rxd RSSI: ");
  DEBUGSERIAL.println(ctrl.rssi, DEC);

  DEBUGSERIAL.fprintln("---------- Got reply:");
  DEBUGSERIAL.println(fromDrive.id);
  DEBUGSERIAL.println(fromDrive.packetCount);
  DEBUGSERIAL.println(fromDrive.dataID);
  
  DEBUGSERIAL.fprint("currentMotor: ");  DEBUGSERIAL.println(drive.currentMotor);
  DEBUGSERIAL.fprint("rpm: ");  DEBUGSERIAL.println(drive.rpm);
  DEBUGSERIAL.fprint("TempMosfet: ");  DEBUGSERIAL.println(drive.tempMosfet);
  DEBUGSERIAL.fprintln("----------");
  return 1;
}


void CtrlLink_EspNowClass::SendTx() {
  if (!bConnected) {
//    ConnectKnown();
//    CheckForConnection();
    return;
  }
  
  // Send data
  struct dataControllerToDrive fromCtrl;
  fromCtrl.id = DL_IDbit_DEST_DRIVE1 | DL_IDbit_DEST_DRIVE2 | DL_ID_SRC_CTRL | DL_ID_TYPE_FAST;
  ctrl.packetCount++;
  ctrl.SetData(fromCtrl);

  msLastMsgSent = millis();
//  Serial.print("Sending # bytes: ");
//  Serial.println(sizeof(fromCtrl));

//  const uint8_t *peer_addr = espConn.peer_addr;
//esp_err_t result = esp_now_send(espConn.peer_addr, reinterpret_cast<uint8_t*>(&fromCtrl), sizeof(fromCtrl));
  esp_err_t result = esp_now_send(espConn.peer_addr, (uint8_t*)&fromCtrl, sizeof(fromCtrl));
  if (result == ESP_OK)
    ; // Serial.println("Send Success");
  else {
    Serial.printf("Send Failed - code: %d\n", result);
    Serial.println(esp_err_to_name(result));
  }

//  int ms1 = millis();
//  DEBUGSERIAL.print(msLastMsgSent);
//  DEBUGSERIAL.fprintln("ms  Packet Sent");
}





void CtrlLink_EspNowClass::ConnectKnown() {
  uint8_t mac[6];

  for (int ii = 0; ii < 6; ++ii ) {
    espConn.peer_addr[ii] = mac[ii] = macThrusterDrive[ii];
//    mac[ii] = macThrusterDrive[ii];
  }
  Serial.printf("Setting peer as %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  espConn.channel = CHANNEL; // pick a channel
  espConn.encrypt = 0; // no encryption
  bConnected = manageConnection();
}


void CtrlLink_EspNowClass::CheckForConnection() {
  if (espConn.channel == CHANNEL) { // check if espConn channel is defined
    Serial.println("Found on channel");
    // `espConn` is defined
    // Add espConn as peer if it has not been added already
    bConnected = manageConnection();
  }
  else
    Serial.println("Failed to Pair");
}


// Scan for espDrives in AP mode
void CtrlLink_EspNowClass::ScanWiFi() {
  Serial.print("Scanning WiFi ....");
  int8_t scanResults = WiFi.scanNetworks();
  // reset on each scan
  bool espDriveFound = 0;
  memset(&espConn, 0, sizeof(espConn));

  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      if (PRINTSCANRESULTS) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(SSID);
        Serial.print(" BSSID[");
        Serial.print(BSSIDstr);
        Serial.print("] (rssi ");
        Serial.print(RSSI);
        Serial.print(")");
        Serial.println();
      }
    }
    
    for (int i = 0; i < scanResults; ++i) {
      // Now check for first 'Slave'
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
      
      delay(10);
      // Check if the current device starts with has correct SSID
      if (SSID.indexOf(SSIDDrive) == 0) {
        // SSID of interest
        Serial.println("Found a Thruster Drive");
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            espConn.peer_addr[ii] = (uint8_t) mac[ii];
          }
        }

        espConn.channel = CHANNEL; // pick a channel
        espConn.encrypt = 0; // no encryption

        espDriveFound = 1;
        // we are planning to have only one espDrive in this example;
        // Hence, break after we find one, to be a bit efficient
        break;
      }
    }
  }

  if (espDriveFound)
    Serial.println("Drive Found");
  else
    Serial.println("Drive Not Found");
  WiFi.scanDelete();    // clean up ram
}


////////////////////////////////////////
// Protected Functions
////////////////////////////////////////


// Check if the espDrive is already paired with the master.
// If not, pair the espDrive with master
bool CtrlLink_EspNowClass::manageConnection() {
  if (espConn.channel == CHANNEL) {
    if (DELETEBEFOREPAIR) {
      deletePeer();
    }

    Serial.println("Connecting ESPNow ...");
    Serial.print("Slave Status: ");
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(espConn.peer_addr);
    if (exists) {
      // Slave already paired.
      Serial.println("Already Paired");
      return true;
    } else {
      // Slave not paired, attempt pair
      esp_err_t result = esp_now_add_peer(&espConn);
      if (result == ESP_OK) {
        Serial.println("Pair success");
        return true;
      }
// const char *esp_err_to_name(esp_err_t code);
// const char *esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen);
      const char* resultStr = esp_err_to_name(result);
      Serial.println(resultStr);
      if (result == ESP_ERR_ESPNOW_EXIST)
        return true;
      else
        return false;

     }
  } else {
    // No espDrive found to process
    Serial.println("No Drive found to process, different channel");
    return false;
  }
}

void CtrlLink_EspNowClass::deletePeer() {
  esp_err_t result = esp_now_del_peer(espConn.peer_addr);
  if (result == ESP_OK)
    Serial.println("Delete Slave Success");
  else {
    Serial.printf("Delete Slave Failed - code: %d\n", result);
    Serial.println(esp_err_to_name(result));
  }
}

int CtrlLink_EspNowClass::GetRxData(uint8_t *dest, int lenMax) {
  if (!bRxNewData)    // no new data rxd
    return 0;
  bRxNewDataReading = 1;    // semaphore to Rx callback
  int len = min(iRxNewDataLen, lenMax);
  memcpy(dest, buffRx, len);
  bRxNewData = 0;
  bRxNewDataReading = 0;
  return len;
}

// callback after data is sent
void CtrlLink_EspNowClass::OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  bTxOK = (status == ESP_NOW_SEND_SUCCESS);

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  if (bTxOK) Serial.print("OnDataSent OK CB to: ");
//  else Serial.print("OnDataSent Failed to: ");
//  Serial.println(macStr);
}

// callback when data is recv
void CtrlLink_EspNowClass::OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (bRxNewDataReading) {    // don't change 'buffRx' if currently being read!
    bRxNewDataMissed = 1;
    return;
  }

  int len = min(data_len, (int)sizeof(buffRx));
  memcpy(buffRx, data, len);
  iRxNewDataLen = len;
  bRxNewData = 1;
  
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  Serial.printf("Recv %d bytes from: %s\n", data_len, macStr);
//  Serial.printf("Data: %d %d %d %d %d   \n", data[0], data[1], data[2], data[3], data[4]);

}




const char* esp_Err_Str(esp_err_t code) {
// const char *esp_err_to_name(esp_err_t code);
// const char *esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen);
  const char* str = NULL;
  switch (code) {
  case ESP_OK:
    str = "OK"; break;
  case ESP_ERR_ESPNOW_NOT_INIT:
    str = "ESPNOW Not Init"; break;
  case ESP_ERR_ESPNOW_ARG:
    str = "Invalid Argument"; break;
  case ESP_ERR_ESPNOW_FULL:
    str = "Peer list full"; break;
  case ESP_ERR_ESPNOW_NO_MEM:
    str = "Out of memory"; break;
  case ESP_ERR_ESPNOW_INTERNAL:
    str = "Internal Error"; break;
  case ESP_ERR_ESPNOW_EXIST:
    str = "Peer Exists"; break;
  case ESP_ERR_ESPNOW_NOT_FOUND:
    str = "Peer not found";  break;
  default:
    str = "Not sure!!";
  }
  return str;
}

#endif  // UseRadio
