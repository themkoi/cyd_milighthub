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
bool isLightMutexTaken = false;

void initLight()
{
    milightClient->prepare(config, deviceId, groupId);
    state = stateStore->get(myBulbId);
    milightClient->updateMode(BULB_MODE_COLOR);
    brightness = state->getBrightness();
    brightnessBeforeOff = state->getBrightness();
}

void releaseMutex()
{
    isLightMutexTaken = false;
    xSemaphoreGive(lightMutex);
}

void waitForAvailable()
{
    isLightMutexTaken = true;
    xSemaphoreTake(lightMutex, portMAX_DELAY);
    delay(200);
}

void turnLightOnBImp()
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

void turnLightOffBImp()
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

void turnLightOnImp()
{
    waitForAvailable();
    state = stateStore->get(myBulbId);
    if (state->isOn() == false)
    {
        milightClient->updateStatus(MiLightStatus::ON);
        Serial.println("Turned light on");
    }
    releaseMutex();
}

void turnLightOffImp()
{
    waitForAvailable();
    state = stateStore->get(myBulbId);
    if (state->isOn() == true)
    {
        milightClient->updateStatus(MiLightStatus::OFF);
        Serial.println("Turned light off");
    }
    releaseMutex();
}

void setBrightnessImp(uint8_t brightness)
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->updateBrightness(brightness);
    }
    releaseMutex();
}

void setHueImp(uint16_t hue)
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->updateHue(hue);
    }
    releaseMutex();
}

void nextModeImp()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->nextMode();
    }
    releaseMutex();
}

void previousModeImp()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->previousMode();
    }
    releaseMutex();
}

void speedUpImp()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->modeSpeedUp();
    }
    releaseMutex();
}

void speedDownImp()
{
    waitForAvailable();
    if (state->isOn() == true)
    {
        milightClient->modeSpeedDown();
    }
    releaseMutex();
}

void lightToggleImp()
{
    waitForAvailable();
    milightClient->toggleStatus();
    releaseMutex();
}

// Queue and mutex handles
QueueHandle_t actionQueue;
SemaphoreHandle_t actionMutex;

void setBrightness(uint8_t brightness)
{
    ActionData actionData;
    actionData.action = LIGHT_BRIGHTNESS;
    actionData.param1 = brightness;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void setHue(uint16_t Hue)
{
    ActionData actionData;
    actionData.action = LIGHT_HUE;
    actionData.param1 = Hue;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void turnLightOffB()
{
    ActionData actionData;
    actionData.action = LIGHT_OFF_B;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void turnLightOnB()
{
    ActionData actionData;
    actionData.action = LIGHT_ON_B;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void turnLightOff()
{
    ActionData actionData;
    actionData.action = LIGHT_OFF;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void turnLightOn()
{
    ActionData actionData;
    actionData.action = LIGHT_ON;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void nextMode()
{
    ActionData actionData;
    actionData.action = LIGHT_MODE_UP;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void previousMode()
{
    ActionData actionData;
    actionData.action = LIGHT_MODE_DOWN;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void speedUp()
{
    ActionData actionData;
    actionData.action = LIGHT_SPEED_UP;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

void speedDown()
{
    ActionData actionData;
    actionData.action = LIGHT_SPEED_DOWN;
    xQueueSend(actionQueue, &actionData, portMAX_DELAY);
}

#define DEBOUNCE_TIME_MS 1

unsigned long lastActionTimestamp = 0;
Action lastAction = NO_ACTION;

void OLED_MANAGERTask(void *pvParameters)
{
    ActionData actionData;
    while (true)
    {
        if (xQueueReceive(actionQueue, &actionData, portMAX_DELAY))
        {
            unsigned long currentMillis = millis();

            if (currentMillis - lastActionTimestamp >= DEBOUNCE_TIME_MS || actionData.action != lastAction)
            {
                switch (actionData.action)
                {
                case LIGHT_ON_B:
                    turnLightOnBImp();
                    break;
                case LIGHT_OFF_B:
                    turnLightOffBImp();
                    break;
                case LIGHT_ON:
                    turnLightOnImp();
                    break;
                case LIGHT_OFF:
                    turnLightOffBImp();
                    break;
                case LIGHT_TOGGLE:
                    lightToggleImp();
                    break;
                case LIGHT_BRIGHTNESS:
                    setBrightnessImp(actionData.param1);
                    break;
                case LIGHT_HUE:
                    setHueImp(actionData.param1);
                    break;
                case LIGHT_MODE_DOWN:
                    previousModeImp();
                    break;
                case LIGHT_MODE_UP:
                    nextModeImp();
                    break;
                case LIGHT_SPEED_DOWN:
                    speedDownImp();
                    break;
                case LIGHT_SPEED_UP:
                    speedUpImp();
                    break;
                default:
                    Serial.println("Unknown action");
                    break;
                }

                xSemaphoreGive(actionMutex);
                lastAction = actionData.action;
                lastActionTimestamp = currentMillis;
            }
            else
            {
                Serial.println("Action debounced");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Adjust delay as needed
    }
}

void initLightManager()
{
    initLight();
    actionQueue = xQueueCreate(1, sizeof(ActionData));
    actionMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
        OLED_MANAGERTask, /* Task function. */
        "OLED_MANAGER",   /* String with name of task. */
        10000,            /* Stack size in words. */
        NULL,             /* Parameter passed as input of the task */
        5,                /* Priority of the task. */
        NULL,             /* Task handle. */
        1                 /* Core where the task should run. */
    );
}