#ifndef HERMES_H
#define HERMES_H

#include "Cloud.hpp"

#include "GoogleCloudIotConfig.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Client.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <MQTT.h>

#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>

using namespace std;

// !!REPLACEME!!
// The MQTT callback function for commands and configuration updates
// Place your message handler code here.
void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
}
///////////////////////////////

// Initialize WiFi and MQTT for this board
WiFiClientSecure *netClient;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
String jwt;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

///////////////////////////////
// Helpers specific to this board
///////////////////////////////
String getDefaultSensor()
{
    return "Wifi: " + String(WiFi.RSSI()) + "db";
}

String getJwt()
{
    Serial.println("Refreshing JWT");
    jwt = device->createJWT(timeClient.getEpochTime(), jwt_exp_secs);
    return jwt;
}

void setupWifi()
{
    delay(100);
    Serial.println("Waiting on time sync...");
    timeClient.begin();
    delay(100);
    timeClient.update();
    return;
}

///////////////////////////////
// Orchestrates various methods from preceeding code.
///////////////////////////////
bool publishTelemetry(String data)
{
    return mqtt->publishTelemetry(data);
}

bool publishTelemetry(const char *data, int length)
{
    return mqtt->publishTelemetry(data, length);
}

bool publishTelemetry(String subfolder, String data)
{
    return mqtt->publishTelemetry(subfolder, data);
}

bool publishTelemetry(String subfolder, const char *data, int length)
{
    return mqtt->publishTelemetry(subfolder, data, length);
}

void mqttLoop()
{
    mqtt->loop();
}

void connect()
{
    mqtt->mqttConnect();
    mqtt->loop();
}

void mqttDisconnect()
{
    mqttClient->disconnect();
}

void setupCloudIoT()
{
    Serial.printf("%s %s %s %s\n", PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID);
    device = new CloudIoTCoreDevice(
        PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID,
        PRIVATE_KEY);

    setupWifi();

    netClient = new WiFiClientSecure();
    //netClient->setCACert(root_cert);

    mqttClient = new MQTTClient(512);
    mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout

    mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);
    mqtt->setUseLts(true);
    mqtt->startMQTT();
}

#endif