#ifndef TSYS01_h
#define TSYS01_h

#include <Arduino.h>

#include <Sensors/Temperature.hpp>
#include <Types.hpp>

class tsys01:public Temperature
{
  public:
    tsys01();
    temperature getTemp();

  private:
    const short _addr = 0x77;
    const short _reset = 0x1E;
    const short _adc_read = 0x00;
    const short _adc_temp_conv = 0x48;
    const short _prom_read = 0xA0;

    uint16_t _calibrationValue[8];
};

#endif