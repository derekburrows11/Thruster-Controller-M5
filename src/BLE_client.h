

void PrintHexData(uint8_t* pData, size_t len);

void BLE_client_setup();
void BLE_client_scan();       // Added by Derek
void BLE_client_connect();    // Added by Derek
void BLE_client_loop();

void remoteCharacteristicWrite(uint8_t* msg, size_t len);
bool remoteCharacteristicRead(std::string& value);

