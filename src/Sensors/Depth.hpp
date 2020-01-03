#ifndef DEPTH_H
#define DEPTH_H

#include <Sensors/Sensor.hpp>
#include <Types.hpp>

using namespace std;

class Depth:public Sensor{
    public:
        virtual depth getDepth() = 0;
};

#endif