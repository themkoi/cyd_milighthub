#ifndef MILIGHTINCLUDES_H
#define MILIGHTINCLUDES_H


#include "defines.h"

#include <SPIFFS.h>

#include <LEDStatus.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <cstdlib>
#include <FS.h>
#include <IntParsing.h>
#include <LinkedList.h>
#include <LEDStatus.h>
#include <GroupStateStore.h>
#include <MiLightRadioConfig.h>
#include <MiLightRemoteConfig.h>
#include <MiLightHttpServer.h>
#include <Settings.h>
#include <MiLightUdpServer.h>
#include <MqttClient.h>
#include <MiLightDiscoveryServer.h>
#include <MiLightClient.h>
#include <BulbStateUpdater.h>
#include <RadioSwitchboard.h>
#include <PacketSender.h>
#include <HomeAssistantDiscoveryClient.h>
#include <TransitionController.h>
#include <ProjectWifi.h>

#include <ESPId.h>

#ifdef ESP8266
  #include <ESP8266mDNS.h>
  #include <ESP8266SSDP.h>
#elif ESP32
  #include "ESP32SSDP.h"
  #include <esp_wifi.h>
  #include <SPIFFS.h>
  #include <ESPmDNS.h>
#endif

#include <vector>
#include <memory>
#include "ProjectFS.h"



extern MiLightClient* milightClient;
extern RadioSwitchboard* radios;
extern PacketSender* packetSender;
extern std::shared_ptr<MiLightRadioFactory> radioFactory;
extern MiLightHttpServer* httpServer;
extern MqttClient* mqttClient;
extern MiLightDiscoveryServer* discoveryServer;
extern uint8_t currentRadioType;

// For tracking and managing group state
extern GroupStateStore* stateStore;
extern BulbStateUpdater* bulbStateUpdater;
extern TransitionController transitions;

#endif