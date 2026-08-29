#include <Arduino.h>
#include <NimBLEDevice.h>
#include "bleHandler.h"

#define SERVICE_UUID        "709bae6b-810c-4dbd-b9d9-544b66d4f54f"
#define CHARACTERISTIC_UUID "d9e368eb-ad7d-4bf6-a9dc-75d9da161805"

EventGroupHandle_t bleEventGroup;

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        xEventGroupSetBits(bleEventGroup, BLE_CONNECTED_BIT);
        xEventGroupClearBits(bleEventGroup, BLE_DISCONNECTED_BIT);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        xEventGroupSetBits(bleEventGroup, BLE_DISCONNECTED_BIT);
        xEventGroupClearBits(bleEventGroup, BLE_CONNECTED_BIT);
        NimBLEDevice::startAdvertising();
    }
};

static MyServerCallbacks serverCallbacks;

void bleTask(void *param) {
    NimBLEDevice::init("milight_hub");

    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pCharacteristic->setValue("Hello World");

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

    NimBLEAdvertisementData advData;
    advData.setName("milight_hub");
    advData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
    pAdvertising->setAdvertisementData(advData);

    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    vTaskDelete(NULL); 
}

void startBleTask() {
    bleEventGroup = xEventGroupCreate();
    xTaskCreatePinnedToCore(
        bleTask,
        "LoopBLE",
        2048,
        NULL,
        1,
        NULL,
        1
    );
}