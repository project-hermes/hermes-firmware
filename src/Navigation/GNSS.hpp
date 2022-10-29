#ifndef GNSS_HPP
#define GNSS_HPP

#include <Arduino.h>
#include <Hal/TinyGPS++.h>
#include <TimeLib.h>

#include <Navigation/Navigation.hpp>
#include <Hal/remora-hal.h>
#include <Types.hpp>

class GNSS : public Navigation
{
public:
    GNSS();

    lat getLat();
    lng getLng();

private:
    HardwareSerial GPSSerial = Serial2;
    TinyGPSPlus gps;

    void parse();
};

#endif