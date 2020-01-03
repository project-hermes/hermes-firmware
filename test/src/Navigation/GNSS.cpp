#include <Navigation/GNSS.hpp>
#include <Types.hpp>

GNSS::GNSS()
{
    pinMode(GPIO_GPS_POWER, OUTPUT);
    digitalWrite(GPIO_GPS_POWER, LOW);
    GPSSerial.begin(9600);
    delay(2000); //TODO this needs to be more dynamic
    unsigned long start = millis();
    while (millis() < start + 5000)
    {
        if (GPSSerial.available() > 0 && gps.encode(GPSSerial.read()))
        {
            if (gps.date.isValid() && gps.time.isValid())
            {
                TimeElements gpsTime = {
                    (uint8_t)gps.time.second(),
                    (uint8_t)gps.time.minute(),
                    (uint8_t)gps.time.hour(),
                    0,
                    (uint8_t)gps.date.day(),
                    (uint8_t)gps.date.month(),
                    (uint8_t)(gps.date.year() - 1970)};
                setTime(makeTime(gpsTime));
            }
        }
    }
}

lat GNSS::getLat()
{
    parse();
    return (lat)gps.location.lat();
}

lng GNSS::getLng()
{
    parse();
    return (lng)gps.location.lng();
}