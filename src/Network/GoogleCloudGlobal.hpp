#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Client.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <MQTT.h>

#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>

#include <Utils.hpp>

WiFiClientSecure *netClient;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
String jwt;
WiFiUDP ntpUDP;
NTPClient *timeClient;

// START CONFIG

// Configuration for NTP
const char *ntp_primary = "pool.ntp.org";
const char *ntp_secondary = "time.nist.gov";

// To get the private key run (where private-key.pem is the ec private key
// used to create the certificate uploaded to google cloud iot):
// openssl ec -in <private-key.pem> -noout -text
// and copy priv: part.
// The key length should be exactly the same as the key length bellow (32 pairs
// of hex digits). If it's bigger and it starts with "00:" delete the "00:". If
// it's smaller add "00:" to the start. If it's too big or too small something
// is probably wrong with your key.
const char *PRIVATE_KEY =
    "ba:6f:28:7f:3d:c1:f5:30:b4:85:fd:74:b3:1b:"
    "31:c2:9f:62:f8:d1:c5:50:aa:9e:b2:88:de:c0:48:"
    "25:bd:99";

// Time (seconds) to expire token += 20 minutes for drift
const int jwt_exp_secs = 3600 * 24; // Maximum 24H (3600*24)

// To get the certificate for your region run:
//   openssl s_client -showcerts -connect mqtt.googleapis.com:8883
// for standard mqtt or for LTS:
//   openssl s_client -showcerts -connect mqtt.2030.ltsapis.goog:8883
// Copy the certificate (all lines between and including ---BEGIN CERTIFICATE---
// and --END CERTIFICATE--) to root.cert and put here on the root_cert variable.

const char *root_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "Bf9tXBi3R91QjAe5uH+hv0lPO4DjvWC0SZPPoA==\n"
    "-----END CERTIFICATE-----\n";

// In case we ever need extra topics
const char *ex_topics[0];
//const int ex_num_topics = 1;
//const char* ex_topics[ex_num_topics] = {
//  "/devices/my-device/tbd/#"
//};

//END CONFIG

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
    if(parseConfig(payload)==-1){
        Serial.println("Failed to parse payload from IOT core state change");
        //TODO need an error logging system at somepoint
    }
    if(writeConfig()==-1){
        Serial.println("Could not write config");
    }
}

String getDefaultSensor()
{
    return "Wifi: " + String(WiFi.RSSI()) + "db";
}

String getJwt()
{
    Serial.println("Refreshing JWT");
    jwt = device->createJWT(timeClient->getEpochTime(), jwt_exp_secs);
    return jwt;
}

void setupWifi()
{
    delay(100);
    Serial.println("Waiting on time sync...");
    timeClient->begin();
    delay(100);
    timeClient->update();
    return;
}

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

void mqttConnect()
{
    mqtt->mqttConnect();
    mqtt->loop();
    mqtt->publishState("I am blue");
    mqtt->loop();
}

void mqttDisconnect()
{
    mqttClient->disconnect();
}

void setupCloudIoT()
{
    Serial.println("setting things up");
    timeClient = new NTPClient(ntpUDP);
    Serial.printf("%s %s %s %s\n", PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID);
    device = new CloudIoTCoreDevice(
        PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID,
        PRIVATE_KEY);

    setupWifi();

    netClient = new WiFiClientSecure();
    //netClient->setCACert(root_cert);

    mqttClient = new MQTTClient(7000);
    mqttClient->setOptions(500, true, 1000); // keepAlive, cleanSession, timeout

    mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);
    mqtt->setUseLts(true);
    mqtt->startMQTT();
}
