#ifndef _CtrlLink_EspNow_h_
#define _CtrlLink_EspNow_h_

#include <inttypes.h>
#include <esp_now.h>

// ESP_NOW_MAX_DATA_LEN defined as 250 in esp_now.h
#define ESPNOW_MAX_RX_SIZE 100



class CtrlLink_EspNowClass {
public:
  void Init();
  bool CheckRx();
  void SendTx();
  uint16_t LastMsgRx() { return msLastMsgRx; }
  void End();

  void ScanWiFi();
  void ConnectKnown();
  void CheckForConnection();
  bool Connected() { return bConnected; }

protected:
  bool manageConnection();
  int GetRxData(uint8_t *dest, int lenMax);

  static void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
  static void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
  static void OnCsiRxd(void *ctx, wifi_csi_info_t *data);

  void deletePeer();


protected:
  uint16_t packetsRxTotal = 0;
  uint16_t packetsRxError = 0;
  uint16_t packetsRxMe = 0;
  uint16_t packetsTx = 0;
  uint16_t msLastMsgSent;
  uint16_t msLastMsgRx;

  static bool bTxOK;
  bool RxOK;
  bool RxTimedOut;
  bool bConnected;

  static bool bRxNewData;
  static bool bRxNewDataReading;    // semaphore to Rx callback
  static bool bRxNewDataMissed;
  static int  iRxNewDataLen;
  static uint8_t buffRx[ESPNOW_MAX_RX_SIZE];

  esp_now_peer_info_t espConn;

};


#endif  // _CtrlLink_EspNow_h_
