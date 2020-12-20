#ifndef UTILS_HPP
#define UTILS_HPP

#include <Arduino.h>

struct Config{
    long updatedAt;
    long createdAt;
    String updatedBy;

    String firmwareVersion;
};

extern Config config;

const String configFilePath = "/config.json";

String remoraID();
int readConfig();
int writeConfig();

#endif