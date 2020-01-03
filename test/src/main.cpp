#include <Arduino.h>

#include <Wake.hpp>

void setup()
{
    Serial.begin(115200);
    delay(1000);

    wake();
    sleep();
}

void loop()
{
}