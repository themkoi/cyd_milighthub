#include "lightControl.h"

BulbId myBulbId(0x1, 0, MiLightRemoteType::REMOTE_TYPE_RGB);

GroupState *state;
uint8_t brightness;

uint8_t brightnessBeforeOff = 100;

const uint16_t deviceId = 0x1;
const uint8_t groupId = 1;
// config is a MiLightRemoteConfig. there are constants available. For example, FUT096Config is for rgbw
const MiLightRemoteConfig *config = &FUT098Config;

void initLight()
{

    milightClient->prepare(config, deviceId, groupId);
    state = stateStore->get(myBulbId);
    brightness = state->getBrightness();
    brightnessBeforeOff = state->getBrightness();
    turnLightOn();
}

void turnLightOn()
{
    state = stateStore->get(myBulbId);
    if (state->isOn() == false)
    {
        milightClient->prepare(config, deviceId, groupId);
        milightClient->updateStatus(MiLightStatus::ON);
        delay(100);
        milightClient->updateBrightness(brightnessBeforeOff);
        for (size_t i = 0; i < 200; i++)
        {
            transitions.loop();
        }
        Serial.println("Turned light on");
    }
}

void turnLightOff()
{
    state = stateStore->get(myBulbId);
    if (state->isOn() == true)
    {
        milightClient->prepare(config, deviceId, groupId);
        brightnessBeforeOff = state->getBrightness();
        uint8_t brightness = 0;
        milightClient->updateBrightness(brightness);
        for (size_t i = 0; i < 200; i++)
        {
            transitions.loop();
        }
        milightClient->updateStatus(MiLightStatus::OFF);
        Serial.println("Turned light off");
    }
}