#ifndef tsys01_h
#define tsys01_h

#include <Arduino.h>

class tsys01
{
  public:
    tsys01();
    double readTemp();

  private:
    const short _addr = 0x77;
    const short _reset = 0x1E;
    const short _adc_read = 0x00;
    const short _adc_temp_conv = 0x48;
    const short _prom_read = 0xA0;

    uint16_t _calibrationValue[8];
};

#endif