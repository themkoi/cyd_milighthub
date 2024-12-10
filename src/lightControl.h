#ifndef LIGHTCONTROL_H
#define LIGHTCONTROL_H

#include "defines.h"


extern BulbId myBulbId;
extern GroupState* state;
extern int brightness;

extern const uint16_t deviceId;
extern const uint8_t groupId;
extern const MiLightRemoteConfig* config;

void initLight();
void turnLightOff();
void turnLightOn();
void setBrightness(uint8_t brightness);


#endif