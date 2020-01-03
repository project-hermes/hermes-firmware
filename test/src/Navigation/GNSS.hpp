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

    void parse()
    {
        unsigned long start = millis();
        while (GPSSerial.available() > 0 && millis() < start + 1000)
        {
            if (gps.encode(GPSSerial.read()))
            {
                if (gps.date.isValid() && gps.time.isValid())
                {
                    TimeElements gpsTime = {
                        (uint8_t)gps.time.second(),
                        (uint8_t)gps.time.minute(),
                        (uint8_t)gps.time.hour(),
                        0,
                        (uint8_t)gps.date.day(),
                        (uint8_t)gps.date.month(),
                        (uint8_t)(gps.date.year() - 1970)};
                    setTime(makeTime(gpsTime));
                }
            }
        }
    }
};

#endif