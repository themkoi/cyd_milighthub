#include "mmWave.h"
#include "defines.h"

HardwareSerial mmWaveSerial(2);

s3km1110 radar;

bool isDetected = true;
bool averageDetect = true;

void initRadar()
{
    mmWaveSerial.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);
    pinMode(MMWAVE_GPIO, INPUT);
    bool isRadarEnabled = radar.begin(mmWaveSerial, Serial);
    Serial.printf(PSTR("Radar status: %s\n"), isRadarEnabled ? "Ok" : "Failed");

    radar.setRadarConfigurationMaximumGates(5);
    radar.setRadarConfigurationMinimumGates(0);
    radar.setRadarConfigurationDelay(10);
    radar.setRadarConfigurationActiveFrameNum(1);
    radar.setRadarConfigurationInactiveFrameNum(10);

    if (isRadarEnabled && radar.readAllRadarConfigs())
    {
        auto config = radar.radarConfiguration;
        // Cast values to unsigned int to match the %u format specifier and prevent build errors
        Serial.printf(PSTR("[Info] Radar config:\n |- Gates  | Min: %u\t| Max: %u\n |- Frames | Detect: %u\t| Disappear: %u\n |- Disappearance delay: %u\n"),
                      (unsigned int)config->detectionGatesMin, 
                      (unsigned int)config->detectionGatesMax, 
                      (unsigned int)config->activeFrameNum, 
                      (unsigned int)config->inactiveFrameNum, 
                      (unsigned int)config->delay);
    }
    delay(1000);

    if (radar.read())
    {
        isDetected = radar.isTargetDetected;
        Serial.println(isDetected);
    }
    delay(500);
}

void setupMMWave()
{
    initRadar();
    milightClient->prepare(config, deviceId, groupId);
    setHue(NATURAL_HUE);
    turnLightOffB();
}

uint32_t lastReading = 0;

static bool previousState = true; 
unsigned long lastStateChange;

#define RESPONSE_TIME 2000
#define RESPONSE_TIME_WHEN_ACTIVE (60000UL * 2)

int responseTime = 5000;
int connectionFailCount = 0;

bool readFailed = false;

void loopMMWave()
{
    static float total = 0.0;
    static int count = 0;
    static float average = 0.0;

    if (readFailed)
    {
        connectionFailCount++;
        Serial.printf(PSTR("No message Attempt: %d/500\n"), connectionFailCount);

        if (connectionFailCount >= 500)
        {
            Serial.println(F("Max failures reached. trying to reset the radar..."));
            initRadar();
            connectionFailCount = 0;
        }
    }

    if (radar.isConnected() && (millis() - lastStateChange >= (unsigned long)responseTime))
    {
        Serial.println(F("Radar connected"));
        lastReading = millis();

        while (millis() - lastReading < 500)
        {
            if (radar.read())
            {
                connectionFailCount = 0;
                readFailed = false;
                isDetected = radar.isTargetDetected;
                Serial.printf(PSTR("Detected: %d\n"), isDetected);
            }
            else
            {
                readFailed = true;
            }
            yield(); // Prevents Task Watchdog Timer reset on ESP boards
        }
    }

    if (isDetected != previousState)
    {
        lastStateChange = millis();
        previousState = isDetected;

        Serial.println(F("Started averaging..."));
        total = 0.0;
        count = 0;
        
        if (isDetected)
        {
            responseTime = RESPONSE_TIME;
        }
        else
        {
            responseTime = RESPONSE_TIME_WHEN_ACTIVE;
        }
    }

    if (millis() - lastStateChange <= (unsigned long)responseTime)
    {
        if (radar.read())
        {
            isDetected = radar.isTargetDetected;
            total += isDetected ? 1.0 : 0.0;
            count++;

            average = total / count;

            Serial.print(F(" | Average: "));
            Serial.println(average);
            Serial.print(F(" | Count: "));
            Serial.println(count);
        }
    }

    if ((millis() - lastStateChange > (unsigned long)responseTime) && count > 0)
    {
        Serial.print(F("Final Average: "));
        Serial.println(average);

        if (average >= 0.99f)
        {
            if (mmwaveState)
            {
                turnLightOnB();
            }
            averageDetect = true;
        }
        else if (average <= 0.01f)
        {
            if (mmwaveState)
            {
                turnLightOffB();
            }
            averageDetect = false;
        }
        else
        {
            lastStateChange = millis();
        }
        
        total = 0.0;
        count = 0;
        average = 0.0;
    }
}