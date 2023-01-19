#include <Arduino.h>
#include <Wake.hpp>
#include "Settings.hpp"

void setup()
{
    Serial.begin(115200);
    delay(100);


#ifdef SERIAL2_DEBUG_OUTPUT
    Serial2.begin(115200);
    delay(100);
    Serial2.setDebugOutput(true);
#endif
    wake();
    sleep(DEFAULT_SLEEP);
}

void loop()
{
}