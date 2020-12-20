#ifndef UTILS_HPP
#define UTILS_HPP

#include <Arduino.h>

struct Config{
    long updatedAt;
    long createdAt;
    String updatedBy;

    String firmwareVersion;
};

Config config;

String configFilePath = "/config.json";

String remoraID();
int readConfig();
int writeConfig(const Config &config);

#endif