/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include <Arduino.h>
#include "BLEDevice.h"
//#include "BLEScan.h"
#include "BLE_client.h"

#include "BMS\BMS_Comms.h"


void ListRemServiceDetails(BLERemoteService* pRemService);



// The remote service we wish to connect to.
//static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID serviceUUID("0000ff00-0000-1000-8000-00805f9b34fb"); // service that LiPo BMS advertises it has!
// LiPo BMS Bluetooth name and address is Xiaoxia, a4:c1:38:97:b9:01

// The characteristic of the remote service we are interested in.
//static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID    charUUID("0000ff02-0000-1000-8000-00805f9b34fb");   // to write
static BLEUUID    charUUID_SPPread ("0000ff01-0000-1000-8000-00805f9b34fb");   // to read
static BLEUUID    charUUID_SPPwrite("0000ff02-0000-1000-8000-00805f9b34fb");   // to write

// Characteristic ff01 is the 'TELINK SPP; Module-Phone, read, notify
// can read BMS info

// Characteristic ff02 is the 'TELINK SPP; Phone-Module, write without response
// can write BMS command

BLEClient* pClient = nullptr;

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristicW;
static BLERemoteCharacteristic* pRemoteCharacteristicR;
static BLEAdvertisedDevice* myDevice;

void remoteCharacteristicWrite(uint8_t* msg, size_t len) {
  pRemoteCharacteristicW->writeValue(msg, len);
}

bool remoteCharacteristicRead(std::string& value) {
  if (!pRemoteCharacteristicR->canRead())
    return 0;
  value = pRemoteCharacteristicR->readValue();    // causes reset...
  return 1;
}

void PrintHexData(uint8_t* pData, size_t len) {
  for (int i = 0; i < len; i++)
    Serial.printf("%.2x ", pData[i]);
  Serial.println();
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
//    Serial.println((char*)pData);
  PrintHexData(pData, length);
  BMSOnRxdPkt(pData, length);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* _pclient) {
    Serial.print("*** OnConnect ***  ");
    Serial.println(connected);
  }

  void onDisconnect(BLEClient* _pclient) {
    Serial.println("**onDisconnect**");
    connected = false;
    pClient = nullptr;
  }
};

bool connectToServer() {
    if (!myDevice) {
      Serial.print("Didn't find a BLE server with service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      return 0;     
    }

    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
//    BLEClient* pClient = BLEDevice::createClient();
    pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remote BLE Server.
    if (!pClient->connect(myDevice)) {  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
      Serial.println("Failed to connect to server");
      return false;
    }
    Serial.print(" - Connected to server.  RSSI: ");
    Serial.println(pClient->getRssi());
    Serial.println(pClient->toString().c_str());


//-------------- read services map
    std::map<std::string, BLERemoteService*>* pMapServices = pClient->getServices();
//    pMapServices->getFirst();

    int num = pMapServices->size();
    Serial.print(" Num Services:  ");
    Serial.println(num);

    for (auto& x: *pMapServices) {
      Serial.print("service: ");
      Serial.println(x.first.c_str());
      ListRemServiceDetails(x.second);
    }
//--------------


    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

//    charUUID = charUUID_SPPwrite;
    
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    pRemoteCharacteristicW = pRemoteService->getCharacteristic(charUUID_SPPwrite);
    pRemoteCharacteristicR = pRemoteService->getCharacteristic(charUUID_SPPread);

    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    if (pRemoteCharacteristicW == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID_SPPwrite.toString().c_str());
      pClient->disconnect();
      return false;
    }
    if (pRemoteCharacteristicR == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID_SPPread.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristics");
    
    std::string value = "Empty";
    // Read the value of the characteristic.
    if(pRemoteCharacteristicW->canRead()) {
//      value = pRemoteCharacteristicW->readValue();    // causes reset...
      Serial.print("The Write characteristic value was: ");
      Serial.println(value.c_str());
    } else
      Serial.println("Couldn't read CharacteristicW");
    if(pRemoteCharacteristicR->canRead()) {
//      value = pRemoteCharacteristicR->readValue();    // causes reset...
      Serial.print("The Read characteristic value was: ");
      Serial.println(value.c_str());
    } else
      Serial.println("Couldn't read CharacteristicR");


//    if(pRemoteCharacteristicW->canNotify())
//      pRemoteCharacteristicW->registerForNotify(notifyCallback);
    if(pRemoteCharacteristicR->canNotify())
      pRemoteCharacteristicR->registerForNotify(notifyCallback);

/* 
    Serial.println("Try to write");
    if(pRemoteCharacteristic->canWrite()) {
      pRemoteCharacteristic->writeValue(msgBMSinfo, sizeof(msgBMSinfo));
      Serial.println("Written: Req basic info and status");
    }
*/

    connected = true;
    return true;
}

bool disconnectFromServer() {
  if (pClient)
    pClient->disconnect();
  else
    Serial.println("Wasn't connected");
}

void ListRemServiceDetails(BLERemoteService* pRemService) {
  Serial.print("  --- ");
  Serial.println(pRemService->toString().c_str());
  pRemService->getCharacteristics();

  //-------------- read characteristics map
    std::map<std::string, BLERemoteCharacteristic*>* pMapCharacteristics = pRemService->getCharacteristics();
    int num = pMapCharacteristics->size();
    Serial.print(" Num Characteristics:  ");
    Serial.println(num);

    for (auto& x: *pMapCharacteristics) {
      Serial.print("  characteristic: ");
      Serial.println(x.first.c_str());
    }

}



/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.print(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = false;    // wait to run 'connect' function
      doScan = false;
    }
    else
      Serial.print("  - no usable service");
    Serial.println();

  } // onResult
}; // MyAdvertisedDeviceCallbacks


void BLE_client_setup() {
  BLEDevice::init("TTgo Watch");
}

void BLE_client_scan() {

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(2000);
  pBLEScan->setWindow(500);
  pBLEScan->setActiveScan(false);  // was true initially
  pBLEScan->start(5, false);
}

void BLE_client_connect() {
      doConnect = true;
}

// Don't loop too quick, currently just overloads requests to BMS once connected!
void BLE_client_loop() {
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
  //    String newValue = "Time since boot: " + String(millis()/1000);
  //    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
  //    uint8_t sec = millis()/1000;

      BMSRequestData();
  //    BMSReadData();    // not working to read direct, use notify
      
    }
    
    doScan = 0;
    if (!connected && doScan) {
      BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
    }
  
}
