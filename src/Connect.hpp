#ifndef CONNECT_HPP
#define CONNECT_HPP
#include <Wake.hpp>
#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>
#include <AutoConnect.h>
#include <Storage/SecureDigital.hpp>
#include <Navigation/GNSS.hpp>
#include <Utils.hpp>
#include <Settings.hpp>
#include <ArduinoJson.h>
#include <Dive.hpp>
#include <secret.hpp>

#define SUCCESS 0
#define FIRMWARE_SIZE_ERROR -2
#define GET_FIRMWARE_ERROR -3
#define CONNECTION_ERROR -4
#define HTTP_BEGIN_ERROR -5

/// @brief Start Remora AP for WIFI configuration
/// @param sd
void startPortal(SecureDigital sd);

/// @brief Upload dives not yet send to recordURL
/// @param sd
/// @return 0 = success;-1 = sd card error;-2 = no index file
int uploadDives(SecureDigital sd);

/// @brief get firmware and install if newer
/// @return
int ota(SecureDigital sd);

/// @brief post dive metadatas
/// @param data json content
/// @return
unsigned long postMetadata(String data);

/// @brief post dive record data
/// @param data json content
/// @return
int postRecordData(String data, unsigned long id);

/// @brief complete silo with id return after post metadata
/// @param records silo  pointer
/// @param diveID ID return by database after post metadata
/// @return updated string
String updateId(String data, unsigned long diveID);
unsigned long checkId(String data);

const int jsonSize = 27000;

#endif