#ifndef CLOUD_H
#define CLOUD_H

#include <Arduino.h>

using namespace std;

class Cloud
{
public:
    virtual int connect() = 0;
    virtual int disconnect() = 0;
    virtual int upload(String payload) = 0;
    virtual int ota() = 0;
};

#endif