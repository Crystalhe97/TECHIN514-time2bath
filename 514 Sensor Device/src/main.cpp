#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SERVICE_UUID        "8a645cd7-c78a-467d-8627-acde6685cf23"
#define CHARACTERISTIC_UUID "9ad53e1b-d2b3-4905-b005-23976f604d2f"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
const int turbidityPin = 1; // 假设浊度传感器连接到GPIO 34
const int oneWireBus = 4;    // DS18B20传感器连接到GPIO 4

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    BLEDevice::init("XIAO_ESP32S3");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());

    sensors.begin(); // 初始化DS18B20传感器

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    if (deviceConnected) {
        sensors.requestTemperatures(); 
        float temperatureC = sensors.getTempCByIndex(0); // 读取温度值
        
        int sensorValue = analogRead(turbidityPin);
        float turbidity = (float)sensorValue * (5.0 / 1023.0); // 示例转换公式

        char buffer[64]; // 缓冲区

        // 根据浊度值的不同，修改消息
        if (turbidity < 3) {
            snprintf(buffer, sizeof(buffer), "Not clear\nTemp: %.2fC", temperatureC);
        } else {
            snprintf(buffer, sizeof(buffer), "Clear\nTemp: %.2fC", temperatureC);
        }

        pCharacteristic->setValue(buffer);
        pCharacteristic->notify();
        Serial.println(buffer);
    }

    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
