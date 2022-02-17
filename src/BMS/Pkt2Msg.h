
#ifndef _Pkt2Msg_h_
#define _Pkt2Msg_h_

//#include <Arduino.h>

class Pkt2Msg {
private:
  int msgPhase;    // 0=waiting for new message, >=1 number packets rxd for message.
  int msgBytesRxd;
  int msgSize;    // expected message size, set on first packet
  int msLastPkt;

  int msgType;
  int msgError;
  bool bHeaderOK;
  bool TailOK;
  uint8_t* pMsg;
  int msgBuffSize;
  bool bGotFullMsg;

public:
  void SetMsg(uint8_t* pMessage, int msgSize);
  void Init();
  void OnRxdPkt(uint8_t* pData, int length);
  bool CheckNewMsg();
  bool CheckTimeOut();
  int GetMsgBytesRxd() { return msgBytesRxd; }
  

};


#endif  // _Pkt2Msg_h_
