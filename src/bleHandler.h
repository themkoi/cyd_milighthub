#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

extern EventGroupHandle_t bleEventGroup;
#define BLE_CONNECTED_BIT    (1 << 0)
#define BLE_DISCONNECTED_BIT (1 << 1)

void startBleTask();

#endif