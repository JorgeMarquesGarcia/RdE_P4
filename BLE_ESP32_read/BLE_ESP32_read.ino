#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("0dd54ba6-6456-4cf9-87d3-e2b3319cb5d1");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("0dd54ba7-6456-4cf9-87d3-e2b3319cb5d1");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static BLEClient* pClient = nullptr;

typedef struct{
  float acc_x, acc_y, acc_z;
  float gyro_x, gyro_y, gyro_z;
} SensorData_t;

SensorData_t sensorData;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.print(sizeof(SensorData_t)); Serial.println(length);

    Serial.print("data: ");
    for (size_t i = 0; i < length; i++) {
      if (pData[i] < 16) Serial.print("0");
      Serial.print(pData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    if (length == sizeof(SensorData_t)) {
      memcpy(&sensorData, pData, sizeof(SensorData_t));
      Serial.println(">>> Datos recibidos por notificación:");
      Serial.print("Acelerómetro X: "); Serial.println(sensorData.acc_x);
      Serial.print("Acelerómetro Y: "); Serial.println(sensorData.acc_y);
      Serial.print("Acelerómetro Z: "); Serial.println(sensorData.acc_z);
      Serial.print("Gyro X: "); Serial.println(sensorData.gyro_x);
      Serial.print("Gyro Y: "); Serial.println(sensorData.gyro_y);
      Serial.print("Gyro Z: "); Serial.println(sensorData.gyro_z);
    } else {
      Serial.print("Tamaño inesperado: "); Serial.println(length);
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    pClient->setMTU(36);  //24 bytes + overhead

  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
    doScan = true;  // ✅ Reconexión automática
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  // ✅ Modificación 2: crear cliente solo si no existe
  if (pClient == nullptr) {
    pClient = BLEDevice::createClient();
    Serial.println(" - Created client");
    pClient->setClientCallbacks(new MyClientCallback());
  }

  // ✅ Modificación 3: desconexión previa si estaba conectado
  if (pClient->isConnected()) {
    Serial.println(" - Already connected, disconnecting first.");
    pClient->disconnect();
    delay(100);
  }

  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  if(pRemoteCharacteristic->canRead()) {
    String valueStr = pRemoteCharacteristic->readValue();
    std::string value = valueStr.c_str();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  // ✅ Modificación 4: evitar múltiple suscripción
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Suscrito correctamente");
  } else {
    Serial.println("Error en la suscripción");
  }


  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server.");
    }
    doConnect = false;
  }
  
  if (!connected && doScan) {
    BLEDevice::getScan()->start(0);  // Reanudar escaneo
  }

  delay(1000);
}
