#include <Navigation/GNSS.hpp>

GNSS::GNSS()
{

parse();
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

void GNSS::parse()
{
    digitalWrite(GPIO_LED2, HIGH);
    pinMode(GPIO_GPS_POWER, OUTPUT);
    digitalWrite(GPIO_GPS_POWER, LOW);

    GPSSerial.begin(9600);
    delay(500); // TODO this needs to be more dynamic
    unsigned long start = millis();
    bool gpsOK = false, timeOK = false;

    pinMode(GPIO_SENSOR_POWER, OUTPUT);
    digitalWrite(GPIO_SENSOR_POWER, LOW);
    delay(10);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);

    tsys01 temperatureSensor = tsys01();
    ms5837 depthSensor = ms5837();
    double temp = temperatureSensor.getTemp();
    double depth = depthSensor.getDepth();

    while (millis() < start + TIME_GPS * 1000 && !gpsOK && !timeOK && depth < MAX_DEPTH_CHECK_WATER)
    {
        if (GPSSerial.available() > 0 && gps.encode(GPSSerial.read()))
        {
            if (gps.date.isValid() && gps.time.isValid())
            {
                log_v("Date: %d/%d/%d", gps.date.day(), gps.date.month(), gps.date.year());
                log_v("Hour: %d:%d:%d", gps.time.hour(), gps.time.minute(), gps.time.second());

                TimeElements gpsTime = {
                    (uint8_t)gps.time.second(),
                    (uint8_t)gps.time.minute(),
                    (uint8_t)gps.time.hour(),
                    0,
                    (uint8_t)gps.date.day(),
                    (uint8_t)gps.date.month(),
                    (uint8_t)(gps.date.year() - 1970)};
                setTime(makeTime(gpsTime));
                timeOK = true;
            }
            if (gps.location.isValid())
            {
                log_v("Position: %f , %f", getLat(), getLng());
                gpsOK = true;
            }
            temp = temperatureSensor.getTemp();
            depth = depthSensor.getDepth();
        }
    }
    digitalWrite(GPIO_LED2, LOW); // turn syn led off when gps connected
    log_d("GPS OK");
}
