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
    "9c:ab:d6:ca:36:19:bc:18:67:27:01:00:14:74:ce:"
    "1e:0d:35:67:9c:90:86:19:75:21:37:aa:0a:c7:f9:"
    "7f:8e";

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
    "MIIBxTCCAWugAwIBAgINAfD3nVndblD3QnNxUDAKBggqhkjOPQQDAjBEMQswCQYD\n"
    "VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzERMA8G\n"
    "A1UEAxMIR1RTIExUU1IwHhcNMTgxMTAxMDAwMDQyWhcNNDIxMTAxMDAwMDQyWjBE\n"
    "MQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\n"
    "QzERMA8GA1UEAxMIR1RTIExUU1IwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATN\n"
    "8YyO2u+yCQoZdwAkUNv5c3dokfULfrA6QJgFV2XMuENtQZIG5HUOS6jFn8f0ySlV\n"
    "eORCxqFyjDJyRn86d+Iko0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUw\n"
    "AwEB/zAdBgNVHQ4EFgQUPv7/zFLrvzQ+PfNA0OQlsV+4u1IwCgYIKoZIzj0EAwID\n"
    "SAAwRQIhAPKuf/VtBHqGw3TUwUIq7TfaExp3bH7bjCBmVXJupT9FAiBr0SmCtsuk\n"
    "miGgpajjf/gFigGM34F9021bCWs1MbL0SA==\n"
    "-----END CERTIFICATE-----\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB4TCCAYegAwIBAgIRKjikHJYKBN5CsiilC+g0mAIwCgYIKoZIzj0EAwIwUDEk\n"
    "MCIGA1UECxMbR2xvYmFsU2lnbiBFQ0MgUm9vdCBDQSAtIFI0MRMwEQYDVQQKEwpH\n"
    "bG9iYWxTaWduMRMwEQYDVQQDEwpHbG9iYWxTaWduMB4XDTEyMTExMzAwMDAwMFoX\n"
    "DTM4MDExOTAzMTQwN1owUDEkMCIGA1UECxMbR2xvYmFsU2lnbiBFQ0MgUm9vdCBD\n"
    "QSAtIFI0MRMwEQYDVQQKEwpHbG9iYWxTaWduMRMwEQYDVQQDEwpHbG9iYWxTaWdu\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEuMZ5049sJQ6fLjkZHAOkrprlOQcJ\n"
    "FspjsbmG+IpXwVfOQvpzofdlQv8ewQCybnMO/8ch5RikqtlxP6jUuc6MHaNCMEAw\n"
    "DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFFSwe61F\n"
    "uOJAf/sKbvu+M8k8o4TVMAoGCCqGSM49BAMCA0gAMEUCIQDckqGgE6bPA7DmxCGX\n"
    "kPoUVy0D7O48027KqGx2vKLeuwIgJ6iFJzWbVsaj8kfSt24bAgAXqmemFZHe+pTs\n"
    "ewv4n4Q=\n"
    "-----END CERTIFICATE-----\n";
// In case we ever need extra topics
const char *ex_topics[0];
// const int ex_num_topics = 1;
// const char* ex_topics[ex_num_topics] = {
//   "/devices/my-device/tbd/#"
// };

// END CONFIG

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
    if (parseConfig(payload) == -1)
    {
        Serial.println("Failed to parse payload from IOT core state change");
        // TODO need an error logging system at somepoint
    }
    if (writeConfig() == -1)
    {
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
    device = new CloudIoTCoreDevice(PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID,PRIVATE_KEY);

    setupWifi();

    netClient = new WiFiClientSecure();
    netClient->setInsecure();
    //netClient->setCACert(root_cert);

    mqttClient = new MQTTClient(7000);

    mqttClient->setOptions(500, true, 1000); // keepAlive, cleanSession, timeout

    mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);

    mqtt->setUseLts(true);
    mqtt->startMQTT();
}
