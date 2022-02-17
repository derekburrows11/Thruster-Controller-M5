
#include <Arduino.h>
#include "Pkt2Msg.h"



void Pkt2Msg::SetMsg(uint8_t* pMessage, int msgSize) {  
  pMsg = pMessage;
  msgBuffSize = msgSize;
}

void Pkt2Msg::Init() {
  msgPhase = 0;
  msgBytesRxd = 0;
  msgSize = 0;
  bHeaderOK = 0;
  TailOK = 0;
  bGotFullMsg = 0;
}

void Pkt2Msg::OnRxdPkt(uint8_t* pData, int length) {
  if (bGotFullMsg)    // if previously got message
    Init();

  // copy packet into message structure
  int sizeRemaining = msgBuffSize - msgBytesRxd;   // don't overrun when copying
  int sizeToCopy = length;
  if (length >= sizeRemaining) {
    sizeToCopy = sizeRemaining;
    bGotFullMsg = 1;
  }
  memcpy(pMsg + msgBytesRxd, pData, sizeToCopy);   // setup as a member uint8_t*

  if (msgPhase == 0) {
    bHeaderOK = (pData[0] == 0xdd);   // should be
    msgType  = pData[1];
    msgError = pData[2];
    msgSize  = pData[3] + 7;   // plus 4 for header and 3 for tail
  }
  if (msgPhase > 0) {
  }

  msgBytesRxd += length;
  msgPhase++;
  msLastPkt = millis();
  CheckNewMsg();

  Serial.printf("Got packet %d, total bytes %d\n", msgPhase, msgBytesRxd);
  Serial.printf("Expected message type %d, size %d\n", msgType, msgSize);
}

bool Pkt2Msg::CheckNewMsg() {
  if (msgBytesRxd > 0)
    if (msgBytesRxd >= msgSize) {
      bGotFullMsg = 1;
      return 1;
    }
  return 0;
}

bool Pkt2Msg::CheckTimeOut() {
  if (msgPhase > 0) {
    if (millis() - msLastPkt > 1000) {
      Init();
      return 0;
    }
  }
  return 1;
}
