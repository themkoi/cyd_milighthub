#include "mmWave.h"

HardwareSerial mmWaveSerial(2);

s3km1110 radar;

bool isDetected = true;

void setupMMWave()
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

    milightClient->prepare(config, deviceId, groupId);

    if (isDetected == true)
    {
        turnLightOn();
    }
}

uint32_t lastReading = 0;

static bool previousState = true; // Tracks the previous state
unsigned long lastStateChange;

#define RESPONSE_TIME 4000
#define RESPONSE_TIME_WHEN_ACTIVE 30000

int responseTime = 5000;

void loopMMWave()
{
    static float total = 0.0;   // Running total of isDetected values
    static int count = 0;       // Count of readings
    static float average = 0.0; // Calculated average

    if (radar.isConnected() && millis() - lastStateChange >= responseTime)
    {
        lastReading = millis();
        while (millis() - lastReading < 500)
        {
            if (radar.read())
            {
                isDetected = radar.isTargetDetected;
                Serial.println("Detected :");
                Serial.println(isDetected);
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
        } else {
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