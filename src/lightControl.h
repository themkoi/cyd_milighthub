#ifndef LIGHTCONTROL_H
#define LIGHTCONTROL_H

#include "defines.h"


extern BulbId myBulbId;
extern GroupState* state;
extern int brightness;

extern const uint16_t deviceId;
extern const uint8_t groupId;
extern const MiLightRemoteConfig* config;

extern SemaphoreHandle_t lightMutex;
extern bool isLightMutexTaken;

typedef enum
{
LIGHT_ON_B,
LIGHT_OFF_B,
LIGHT_ON,
LIGHT_OFF,
LIGHT_TOGGLE,
LIGHT_BRIGHTNESS,
LIGHT_HUE,
LIGHT_MODE_UP,
LIGHT_MODE_DOWN,
LIGHT_SPEED_UP,
LIGHT_SPEED_DOWN,
NO_ACTION
} Action;

typedef struct
{
    Action action;
    uint16_t param1;
} ActionData;

void initLightManager();

void turnLightOff();
void turnLightOn();
void turnLightOffB();
void turnLightOnB();
void setBrightness(uint8_t brightness);
void setHue(uint16_t hue);
void nextMode();
void previousMode();
void speedUp();
void speedDown();



#endif