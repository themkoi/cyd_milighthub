#include "mmWave.h"

HardwareSerial mmWaveSerial(2);

s3km1110 radar;

bool isDetected = true;

void initRadar()
{
    mmWaveSerial.begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);
    pinMode(MMWAVE_GPIO, INPUT);
    bool isRadarEnabled = radar.begin(mmWaveSerial, Serial);
    Serial.printf("Radar status: %s\n", isRadarEnabled ? "Ok" : "Failed");

    radar.setRadarConfigurationMaximumGates(5);
    radar.setRadarConfigurationMinimumGates(0);
    radar.setRadarConfigurationDelay(5);
    radar.setRadarConfigurationActiveFrameNum(5);
    radar.setRadarConfigurationInactiveFrameNum(20);

    if (isRadarEnabled && radar.readAllRadarConfigs())
    {
        auto config = radar.radarConfiguration;
        Serial.printf("[Info] Radar config:\n |- Gates  | Min: %u\t| Max: %u\n |- Frames | Detect: %u\t| Disappear: %u\n |- Disappearance delay: %u\n",
                      config->detectionGatesMin, config->detectionGatesMax, config->activeFrameNum, config->inactiveFrameNum, config->delay);
    }
    delay(1000);

    if (radar.read())
    {
        // Get radar info
        isDetected = radar.isTargetDetected;
        int16_t targetDistance = radar.distanceToTarget;
        Serial.println(isDetected);
    }
    delay(500);
}

void setupMMWave()
{
    initRadar();
    milightClient->prepare(config, deviceId, groupId);

    if (isDetected == true)
    {
        turnLightOn();
        setHue(NATURAL_HUE);
    }
}

uint32_t lastReading = 0;

static bool previousState = true; // Tracks the previous state
unsigned long lastStateChange;

#define RESPONSE_TIME 2000
#define RESPONSE_TIME_WHEN_ACTIVE 60000 * 2

int responseTime = 5000;
int connectionFailCount = 0;

bool readFailed = false;

void loopMMWave()
{
    static float total = 0.0; 
    static int count = 0;
    static float average = 0.0; 
    if (readFailed == true)
    {
        connectionFailCount++;
        Serial.printf("No message Attempt: %d/500\n", connectionFailCount);
        if (connectionFailCount > 1)
        {
        }

        if (connectionFailCount >= 500)
        {
            Serial.println("Max failures reached. trying to reset the radar...");
            initRadar();
            connectionFailCount = 0;
        }
    }

    if (radar.isConnected() && millis() - lastStateChange >= responseTime)
    {
        Serial.println("Radar connected");
        lastReading = millis();

        while (millis() - lastReading < 500)
        {
            if (radar.read())
            {
                connectionFailCount = 0;
                readFailed = false;
                isDetected = radar.isTargetDetected;
                Serial.printf("Detected: %d\n", isDetected);
            } else {
                readFailed = true;
            }
        }
    }

    if (isDetected != previousState)
    {
        lastStateChange = millis();
        previousState = isDetected;

        Serial.println("Started averaging...");
        total = 0.0;
        count = 0;
        if (isDetected == true)
        {
            responseTime = RESPONSE_TIME;
        }
        else
        {
            responseTime = RESPONSE_TIME_WHEN_ACTIVE;
        }
    }
    while (millis() - lastStateChange <= responseTime)
    {

        if (radar.read() && millis() - lastStateChange <= responseTime)
        {

            isDetected = radar.isTargetDetected;
            total += isDetected ? 1.0 : 0.0;
            count++;

            average = total / count;

            Serial.print(" | Average: ");
            Serial.println(average);
            Serial.print(" | Count: ");
            Serial.println(count);
        }
    }

    if (millis() - lastStateChange > responseTime && count > 0)
    {
        Serial.print("Final Average: ");
        Serial.println(average);

        if (average == 1.00)
        {
            turnLightOn();
        }
        else if (average == 0.00)
        {
            turnLightOff();
        }
        else
        {
            lastStateChange = millis();
            total = 0.0;
            count = 0;
        }
        total = 0.0;
        count = 0;
        average = 0.0;
    }
}