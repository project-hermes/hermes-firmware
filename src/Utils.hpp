#ifndef UTILS_HPP
#define UTILS_HPP

#include <Arduino.h>

struct Config{
    long updatedAt;
    long createdAt;
    String updatedBy;

    String currentFirmwareVersion;
    String targetFirmwareVersion;
};

static Config config = {};

const String configFilePath = "/config.json";

String remoraID();
int parseConfig(String data);
int readConfig();
int writeConfig();


#endif