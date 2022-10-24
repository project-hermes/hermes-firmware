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

/// @brief 
/// @param data json content 
/// @param metadata 0 = recordURL; 1 = metadataURL;
/// @return 
int post(String data, bool metadata = 0);

#endif