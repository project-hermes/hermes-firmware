#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <esp_log.h>

#include <hal/MS5837.hpp>
#include <Types.hpp>

ms5837::ms5837()
{
    // Reset the MS5837, per datasheet
    Wire.beginTransmission(_addr);
    Wire.write(_reset);
    Wire.endTransmission();

    // Wait for reset to complete
    delay(10);

    // Read calibration values and CRC
    for (uint8_t i = 0; i < 7; i++)
    {
        Wire.beginTransmission(_addr);
        Wire.write(_prom_read + i * 2);
        Wire.endTransmission();

        Wire.requestFrom(_addr, 2);
        _msCalibrationValue[i] = (Wire.read() << 8) | Wire.read();
#ifdef DEBUG
        Serial.printf("calibration value %d: %d\n", i, _msCalibrationValue[i]);
#endif
    }

    // Verify that data is correct with CRC
    uint8_t crcRead = _msCalibrationValue[0] >> 12;
    uint8_t crcCalculated = crc4(_msCalibrationValue);
    if (crcCalculated != crcRead)
    {
        Serial.printf("CRC does not match!!!\n");
        Serial.printf("Got %u but expected %u\n", crcCalculated, crcRead);
    }
}

uint8_t ms5837::crc4(uint16_t check_code[])
{
    uint16_t n_rem = 0;

    check_code[0] = ((check_code[0]) & 0x0FFF);
    check_code[7] = 0;

    for (uint8_t i = 0; i < 16; i++)
    {
        if (i % 2 == 1)
        {
            n_rem ^= (uint16_t)((check_code[i >> 1]) & 0x00FF);
        }
        else
        {
            n_rem ^= (uint16_t)(check_code[i >> 1] >> 8);
        }
        for (uint8_t n_bit = 8; n_bit > 0; n_bit--)
        {
            if (n_rem & 0x8000)
            {
                n_rem = (n_rem << 1) ^ 0x3000;
            }
            else
            {
                n_rem = (n_rem << 1);
            }
        }
    }

    n_rem = ((n_rem >> 12) & 0x000F);

    return n_rem ^ 0x00;
}

void ms5837::readValues()
{
    // Request Raw Pressure conversion
    Wire.beginTransmission(_addr);
    Wire.write(_d1_8192);
    Wire.endTransmission();

    // Max conversion time per datasheet
    delay(20);

    Wire.beginTransmission(_addr);
    Wire.write(_adc_read);
    Wire.endTransmission();

    Wire.requestFrom(_addr, 3);
    _rawPres = 0;
    _rawPres = Wire.read();
    _rawPres = (_rawPres << 8) | Wire.read();
    _rawPres = (_rawPres << 8) | Wire.read();

    // Request Raw Temperature conversion
    Wire.beginTransmission(_addr);
    Wire.write(_d2_8192);
    Wire.endTransmission();

    delay(20); // Max conversion time per datasheet

    Wire.beginTransmission(_addr);
    Wire.write(_adc_read);
    Wire.endTransmission();

    Wire.requestFrom(_addr, 3);
    _rawTemp = 0;
    _rawTemp = Wire.read();
    _rawTemp = (_rawTemp << 8) | Wire.read();
    _rawTemp = (_rawTemp << 8) | Wire.read();
#ifdef DEBUG
    Serial.printf("rawTemp: %u rawPressure: %u\n", _rawTemp, _rawPres);
#endif
}

void ms5837::computeTemp()
{
    readValues();

    _deltaTemp = _rawTemp - _msCalibrationValue[5] * pow(2, 8);
    _temp = (2000 + _deltaTemp * _msCalibrationValue[6] / pow(2, 23));
}

void ms5837::computePressure()
{
    computeTemp();

    int64_t offset = _msCalibrationValue[2] * pow(2, 16) + (_msCalibrationValue[4] * _deltaTemp) / pow(2, 7);
    int64_t sensitivity = _msCalibrationValue[1] * pow(2, 15) + (_msCalibrationValue[3] * _deltaTemp) / pow(2, 8);
    _pressure = ((_rawPres * sensitivity / pow(2, 21) - offset) / pow(2, 13)) / 10;
}

pressure ms5837::getPressure()
{
    computePressure();
    return _pressure;
}

temperature ms5837::getTemp()
{
    computeTemp();
    return _temp / 100.0;
}

depth ms5837::getDepth()
{
    computePressure();
    return ((_pressure * _pa) - 101300) / (_fluidDensity * 9.80665f);
}