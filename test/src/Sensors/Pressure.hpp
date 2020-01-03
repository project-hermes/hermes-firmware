#ifndef PRESSURE_H
#define PRESSURE_H

#include <Sensors/Sensor.hpp>
#include <Types.hpp>

using namespace std;

class Pressure : public Sensor{
    public:
        virtual pressure getPressure() = 0;
};

#endif