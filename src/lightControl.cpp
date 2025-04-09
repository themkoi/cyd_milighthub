#include "lightControl.h"

const uint16_t deviceId = 4276;
const uint8_t groupId = 0;

BulbId myBulbId(deviceId, groupId, MiLightRemoteType::REMOTE_TYPE_RGB);

GroupState *state;
int brightness;

uint8_t brightnessBeforeOff = 100;

// config is a MiLightRemoteConfig. there are constants available. For example, FUT096Config is for rgbw
const MiLightRemoteConfig *config = &FUT098Config;

SemaphoreHandle_t lightMutex;

void initLight()
{
    milightClient->prepare(config, deviceId, groupId);
    state = stateStore->get(myBulbId);
    milightClient->updateMode(BULB_MODE_COLOR);
    brightness = state->getBrightness();
    brightnessBeforeOff = state->getBrightness();
}

void releaseMutex() {
    xSemaphoreGive(lightMutex);
}

void waitForAvailable() {
    xSemaphoreTake(lightMutex, portMAX_DELAY);
    delay(500);
}


void turnLightOn()
{
    waitForAvailable();
    state = stateStore->get(myBulbId);
    if (state->isOn() == false)
    {
        milightClient->updateStatus(MiLightStatus::ON);
        delay(100);
        milightClient->updateBrightness(brightnessBeforeOff);
        for (size_t i = 0; i < 200; i++)
        {
            transitions.loop();
        }
        Serial.println("Turned light on");
    }
    releaseMutex();
}

void turnLightOff()
{
    waitForAvailable();
    state = stateStore->get(myBulbId);
    if (state->isOn() == true)
    {
        brightnessBeforeOff = state->getBrightness();
        brightness = 0;
        milightClient->updateBrightness(brightness);
        for (size_t i = 0; i < 200; i++)
        {
            transitions.loop();
        }
        milightClient->updateStatus(MiLightStatus::OFF);
        Serial.println("Turned light off");
    }
    releaseMutex();
}

void setBrightness(uint8_t brightness)
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->updateBrightness(brightness);
    }
    releaseMutex();
}

void setHue(uint16_t hue)
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->updateHue(hue);
    }
    releaseMutex();
}

void nextMode()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->nextMode();
    }
    releaseMutex();
}

void previousMode()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->previousMode();
    }
    releaseMutex();
}

void speedUp()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->modeSpeedUp();
    }
    releaseMutex();
}

void speedDown()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->modeSpeedDown();
    }
    releaseMutex();
}