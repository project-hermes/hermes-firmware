#include <WiFi.h>
#include <mbedtls/md.h>

#include <Storage/SecureDigital.hpp>
#include <ArduinoJson.h>

#include <Arduino.h>

#include <Utils.hpp>

String remoraID()
{
    byte shaResult[32];
    WiFi.mode(WIFI_MODE_STA);
    String unhashed_id = WiFi.macAddress();
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    const size_t payloadLength = strlen(unhashed_id.c_str());

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)unhashed_id.c_str(), payloadLength);
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    String hash;
    for (int i = 0; i < sizeof(shaResult); i++)
    {
        char str[3];

        sprintf(str, "%02x", (int)shaResult[i]);
        hash = hash + str;
    }

    return hash;
}

int parseConfig(String data)
{
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, data);
    if (error)
    {
        Serial.println("Could not deserialize config file!");
        return -1;
    }

    config.updatedAt = doc["updatedAt"];
    config.createdAt = doc["createdAt"];
    config.updatedBy = doc["updatedBy"].as<String>();
    config.currentFirmwareVersion = doc["currentFirmwareVersion"].as<String>();
    config.targetFirmwareVersion = doc["targetFirmwareVersion"].as<String>();

    return 0;
}

int readConfig()
{
    SecureDigital sd = SecureDigital();
    StaticJsonDocument<512> doc;

    if (sd.findFile(configFilePath) == -1)
    {
        //TODO I am not sure if I need to check for var init of the config obj
        if (sd.touchFile(configFilePath) == -1)
        {
            Serial.println("Failed to setup create a blank config file");
            return -1;
        }

        if (writeConfig() == -1)
        {
            Serial.println("Failed to write blank config file");
            return -1;
        }
    }

    String data = sd.readFile(configFilePath);
    if (data == "")
    {
        Serial.println("Error reading congfig file!");
        return -1;
    }

    DeserializationError error = deserializeJson(doc, data);
    if (error)
    {
        Serial.println("Could not deserialize config file!");
        return -1;
    }

    config.updatedAt = doc["updatedAt"];
    config.createdAt = doc["createdAt"];
    config.updatedBy = doc["updatedBy"].as<String>();
    config.currentFirmwareVersion = doc["currentFirmwareVersion"].as<String>();
    config.targetFirmwareVersion = doc["targetFirmwareVersion"].as<String>();

    return 0;
}

int writeConfig()
{
    SecureDigital sd = SecureDigital();

    if (sd.findFile(configFilePath) == 0)
    {
        if (sd.deleteFile(configFilePath) == -1)
        {
            Serial.println("Failed to delete config file for update");
            return -1;
        }
    }

    StaticJsonDocument<512> doc;

    doc["updatedAt"] = config.updatedAt;
    doc["createdAt"] = config.createdAt;
    doc["updatedBy"] = config.updatedBy;
    doc["currentFirmwareVersion"] = config.currentFirmwareVersion;
    doc["targetFirmwareVersion"] = config.targetFirmwareVersion;

    String buffer;
    serializeJson(doc, buffer);

    if (sd.writeFile(configFilePath, buffer) == -1)
    {
        Serial.println("Failed to write config file");
        return -1;
    }

    return 0;
}