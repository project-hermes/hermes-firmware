#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <Sensors/Sensor.hpp>
#include <Types.hpp>

using namespace std;

class Temperature: public Sensor{
    public:
        virtual temperature getTemp() = 0;

        temperature toFahrenheit(temperature t){
            return (t * (9 / 5)) + 32;
        }

        temperature toCelsius(temperature t){
            return (t - 32) * (5 / 9);
        }

        temperature toKelvin(temperature t){
            return t + 273.15;
        }
};

#endif