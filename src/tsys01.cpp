#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "tsys01.h"

tsys01::tsys01()
{
    Wire.beginTransmission(_addr);
    Wire.write(_reset);
    Wire.endTransmission();

    delay(10);

    for (uint8_t i = 0; i < 8; i++)
    {
        Wire.beginTransmission(_addr);
        Wire.write(_prom_read + i * 2);
        Wire.endTransmission();

        Wire.requestFrom(_addr, 2);
        _calibrationValue[i] = (Wire.read() << 8) | Wire.read();
    }
}

double tsys01::readTemp()
{
    Wire.beginTransmission(_addr);
    Wire.write(_adc_temp_conv);
    Wire.endTransmission();

    delay(10);

    Wire.beginTransmission(_addr);
    Wire.write(_adc_read);
    Wire.endTransmission();

    Wire.requestFrom(_addr, 3);
    uint32_t data = 0;
    data = Wire.read();
    data = (data << 8) | Wire.read();
    data = (data << 8) | Wire.read();

    data = data / 256;

    return (-2) * float(_calibrationValue[1]) / 1000000000000000000000.0f * pow(data, 4) +
           4 * float(_calibrationValue[2]) / 10000000000000000.0f * pow(data, 3) +
           (-2) * float(_calibrationValue[3]) / 100000000000.0f * pow(data, 2) +
           1 * float(_calibrationValue[4]) / 1000000.0f * data +
           (-1.5) * float(_calibrationValue[5]) / 100;
}