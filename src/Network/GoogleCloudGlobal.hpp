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
    "MIIFdzCCBF+gAwIBAgIQRz3TEzkUgfgKAAAAASuB0jANBgkqhkiG9w0BAQsFADBG\n"
    "MQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\n"
    "QzETMBEGA1UEAxMKR1RTIENBIDFDMzAeFw0yMTEyMjcwODE1NTZaFw0yMjAzMjEw\n"
    "ODE1NTVaMB4xHDAaBgNVBAMTE21xdHQuZ29vZ2xlYXBpcy5jb20wggEiMA0GCSqG\n"
    "SIb3DQEBAQUAA4IBDwAwggEKAoIBAQCzLFulAevY43wp5A0kJAKLP6Eb5qAigrAq\n"
    "1FCbk+QRuSsr+O4vP8dXYWwP08aNRqWTmCOaFmhUCmR9CGbkpnxwdOPRB89+zagX\n"
    "sKYKBSTS10DTxdLqt0h4yC4uKC52zzNHSi86DNhDE4/xLTLB4YA1LDuFSHdLMHDN\n"
    "ZRJhTN8kw01ULU7U7i16bSFy4utL1hwaBRRiDgYP8P3a19+jdmGl81p5fBX3Ucup\n"
    "xYZL/GxlJvqA4kh5I5dAXIkOzG4CGrLG5Bbf5zupSZOp0UqOVtfsdIU/4NiXGtBt\n"
    "6a1KIcOVh/VTFGjXQnXtHom/4QLC6/XXR2BaTUtPUYDrSj2KVeQJAgMBAAGjggKH\n"
    "MIICgzAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDAYDVR0T\n"
    "AQH/BAIwADAdBgNVHQ4EFgQUmVOHZQcYUbRhWDjeU/cRBR0h2egwHwYDVR0jBBgw\n"
    "FoAUinR/r4XN7pXNPZzQ4kYU83E1HScwagYIKwYBBQUHAQEEXjBcMCcGCCsGAQUF\n"
    "BzABhhtodHRwOi8vb2NzcC5wa2kuZ29vZy9ndHMxYzMwMQYIKwYBBQUHMAKGJWh0\n"
    "dHA6Ly9wa2kuZ29vZy9yZXBvL2NlcnRzL2d0czFjMy5kZXIwOAYDVR0RBDEwL4IT\n"
    "bXF0dC5nb29nbGVhcGlzLmNvbYIYbXF0dC1tdGxzLmdvb2dsZWFwaXMuY29tMCEG\n"
    "A1UdIAQaMBgwCAYGZ4EMAQIBMAwGCisGAQQB1nkCBQMwPAYDVR0fBDUwMzAxoC+g\n"
    "LYYraHR0cDovL2NybHMucGtpLmdvb2cvZ3RzMWMzL3pkQVR0MEV4X0ZrLmNybDCC\n"
    "AQUGCisGAQQB1nkCBAIEgfYEgfMA8QB2ACl5vvCeOTkh8FZzn2Old+W+V32cYAr4\n"
    "+U1dJlwlXceEAAABffsszdAAAAQDAEcwRQIhAIWCDTvJB8Mzzx1dk3AmgtpZu7qG\n"
    "Htgb4LmgeURsJeMAAiBC2PIWd130X8rU70vBNFazH23dHlJ8sbqXFEVGc9azHgB3\n"
    "AN+lXqtogk8fbK3uuF9OPlrqzaISpGpejjsSwCBEXCpzAAABffsszd8AAAQDAEgw\n"
    "RgIhAKc2WvuXFj8RB6rAqFz8o5WOW/dVodteKtelURH2bFlqAiEAh0mwNElQEUSS\n"
    "d1tEE0oC/IPsUc8ioBrDJ0CfQFaDbX4wDQYJKoZIhvcNAQELBQADggEBAJ+dUen7\n"
    "piFwp7KRNY728rfC6jDTveJyLi98sRz/I6K2+VMVXGQrYgLDi9rpCNrpbTQB1NwF\n"
    "IB+ZjH/hW3W5Ahs+cL+v/T6FgeeMqtUJGJbxWqEekNVBYzRnp9iKCZTm/F/PGNsf\n"
    "AH+d4JRiP76zPxwcBNQNIYHmbUjV+foqHIrZ5DjbJUwx9YFibQf3rRgdCPZAoM33\n"
    "8JfkgMnIZl1dYlKH7FRdK6uHaHhm+WdMGY5xpSXT3jMJpUmgLRlOzFZgUjtFA78w\n"
    "O6lCwiGJxNhnXtvLH9U3F+JOcENFLfKDZs8tFk8lbx8M2fK1Y94ZDBrubw3ebOec\n"
    "MjhWkLeL63YbuOc=\n"
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
    device = new CloudIoTCoreDevice(
        PROJECT_ID, REGION_NAME, REGISTRY_ID, DEVICE_ID,
        PRIVATE_KEY);

    setupWifi();

    netClient = new WiFiClientSecure();
    // netClient->setCACert(root_cert);

    mqttClient = new MQTTClient(7000);

    mqttClient->setOptions(500, true, 1000); // keepAlive, cleanSession, timeout

    mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);

    mqtt->setUseLts(true);
    mqtt->startMQTT();
}
