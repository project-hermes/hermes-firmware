#ifndef HERMES_H
#define HERMES_H

#include "Cloud.hpp"

#include "GoogleCloudIotConfig.h"

#include "time.h"

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
unsigned long iat = 0;
String jwt;

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
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long milliseconds = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
    jwt = device->createJWT(1602554024, jwt_exp_secs);
    Serial.println(milliseconds);
    Serial.println(jwt.c_str());
    return jwt;
}

void setupWifi()
{
    delay(100);
    configTime(0, 0, ntp_primary, ntp_secondary);
    Serial.println("Waiting on time sync...");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t milliseconds = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
    while (milliseconds < 1602554024)
    {
        gettimeofday(&tv, NULL);
        milliseconds = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
        delay(100);
        Serial.printf("Now is %lu needs to be greater than 1510644967\n", milliseconds);
    }
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

void connect()
{
    mqtt->mqttConnect();
}

void setupCloudIoT()
{
    Serial.printf("%s %s %s %s\n",PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID);
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