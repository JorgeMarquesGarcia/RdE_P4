#include <ArduinoBLE.h>
#include <Wire.h>
#include <SPI.h>
#include <Arduino_LSM9DS1.h>

typedef struct {
  float acc_x, acc_y, acc_z;
  float gyro_x, gyro_y, gyro_z;
} SensorData_t;


SensorData_t SensorData;
unsigned char buffer[sizeof(SensorData)]; 
void ReadData();

BLEService imuService("0dd54ba6-6456-4cf9-87d3-e2b3319cb5d1"); // BLE Custom generation of UUID

// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
//BLEByteCharacteristic accCharacteristic("0dd54ba7-6456-4cf9-87d3-e2b3319cb5d1", BLERead | BLENotify, sizeof(SensorData)); //Lectura y actualizaciones 
BLECharacteristic accCharacteristic("0dd54ba7-6456-4cf9-87d3-e2b3319cb5d1", 
                                    BLERead | BLENotify, sizeof(SensorData));


//BLEByteCharacteristic gyrCharacteristic("0dd54ba8-6456-4cf9-87d3-e2b3319cb5d1", BLERead | BLENotify, sizeof(float) *3); //Lectura y actualizaciones 




void setup() {
  Serial.begin(9600);
  while (!Serial);

  // set LED's pin to output mode
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);         // when the central disconnects, turn off the LED
  
  //begin IMU initialization
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while(1);
  }
  IMU.setContinuousMode(); // Set continuous mode for the IMU

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");

    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("Nano 33 BLE Sense");
  BLE.setAdvertisedService(imuService);

  // add the characteristic to the service
  imuService.addCharacteristic(accCharacteristic);

  // add service
  BLE.addService(imuService);

  // set the initial value for the characteristic:
  //switchCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("BLE LED Peripheral");
}
char hexString[4];  // Para almacenar el valor en hexadecimal como cadena

void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, HIGH);            // turn on the LED to indicate the connection

    // while the central is still connected to peripheral:
    while (central.connected()) {
      ReadData();
      memcpy(buffer, &SensorData, sizeof(SensorData));
      accCharacteristic.writeValue(buffer, sizeof(SensorData_t));
      // Imprimir el buffer en formato HEX para ver su contenido
      for (size_t i = 0; i < sizeof(SensorData); i++) {
        sprintf(hexString, "%02X ", buffer[i]);  // Convierte el byte en una cadena
        Serial.print(hexString);  // Imprime la cadena
      }
      Serial.println();
    
   
    delay(500);
    }
  }
      // if the remote device wrote to the characteristic,
      // use the value to control the LED:
      /*
      if (accCharacteristic.write()) {
        switch (switchCharacteristic.notify()) {   // any value other than 0
          case 01:
            Serial.println("Red LED on");
            digitalWrite(LEDR, LOW);            // will turn the LED on
            digitalWrite(LEDG, HIGH);         // will turn the LED off
            digitalWrite(LEDB, HIGH);         // will turn the LED off
            break;
          case 02:
            Serial.println("Green LED on");
            digitalWrite(LEDR, HIGH);         // will turn the LED off
            digitalWrite(LEDG, LOW);        // will turn the LED on
            digitalWrite(LEDB, HIGH);        // will turn the LED off
            break;
          case 03:
            Serial.println("Blue LED on");
            digitalWrite(LEDR, HIGH);         // will turn the LED off
            digitalWrite(LEDG, HIGH);       // will turn the LED off
            digitalWrite(LEDB, LOW);         // will turn the LED on
            break;
          default:
            Serial.println(F("LEDs off"));
            digitalWrite(LEDR, HIGH);          // will turn the LED off
            digitalWrite(LEDG, HIGH);        // will turn the LED off
            digitalWrite(LEDB, HIGH);         // will turn the LED off
            break;
        }
      }
    }
*/
    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, LOW);         // when the central disconnects, turn off the LED
    //digitalWrite(LEDR, HIGH);          // will turn the LED off
    //digitalWrite(LEDG, HIGH);        // will turn the LED off
    //digitalWrite(LEDB, HIGH);         // will turn the LED off
  }




void ReadData(){
  IMU.readAcceleration(SensorData.acc_x, SensorData.acc_y, SensorData.acc_z);
  IMU.readGyroscope(SensorData.gyro_x, SensorData.gyro_y, SensorData.gyro_z); 
}
