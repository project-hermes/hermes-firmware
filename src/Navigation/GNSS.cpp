#include <Navigation/GNSS.hpp>
#include "GNSS.hpp"

GNSS::GNSS()
{
    // parse();
    GPSSerial.begin(9600);
    delay(100);
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

    ms5837 depthSensor = ms5837();
    double depth = depthSensor.getDepth();
    /*
        char nextData;
        while (1)
        {
            while (GPSSerial.available())
            {
                nextData = GPSSerial.read();
                Serial1.write(nextData);
                if (gps.encode(nextData))
                {
                    Serial1.write("\n");
                    if (gps.time.isUpdated())
                        log_d("Hour: %d:%d:%d", gps.time.hour(), gps.time.minute(), gps.time.second());
                }
            }
        }
        */
    while (millis() < start + TIME_GPS * 1000 && (!gpsOK || !timeOK) && depth < MAX_DEPTH_CHECK_GPS)
    {

        if (GPSSerial.available() > 0 && gps.encode(GPSSerial.read()))
        {
            if (gps.date.isValid() && gps.time.isValid())
            {
                log_d("Date: %d/%d/%d", gps.date.day(), gps.date.month(), gps.date.year());
                log_d("Hour: %d:%d:%d", gps.time.hour(), gps.time.minute(), gps.time.second());

                TimeElements gpsTime = {
                    (uint8_t)gps.time.second(),
                    (uint8_t)gps.time.minute(),
                    (uint8_t)gps.time.hour(),
                    0,
                    (uint8_t)gps.date.day(),
                    (uint8_t)gps.date.month(),
                    (uint8_t)(gps.date.year() - 1970)};
                setTime(makeTime(gpsTime));
                if (year() > 1970)
                    timeOK = true;
            }
            if (gps.location.isValid())
            {
                log_v("Position: %f , %f", getLat(), getLng());
                gpsOK = true;
            }
            depth = depthSensor.getDepth();
        }
    }

    digitalWrite(GPIO_LED2, LOW); // turn syn led off when gps connected
    log_d("GPS OK");
}
time_t GNSS::getTime()
{
    struct tm tm;  // tm struct to store date time from gps
    char nextData; // char received from gps
    bool timeReady = false;

    pinMode(GPIO_GPS_POWER, OUTPUT);
    digitalWrite(GPIO_GPS_POWER, LOW);

    while (timeReady == false)
    {
        while (GPSSerial.available())
        {
            nextData = GPSSerial.read();
            // Serial1.write(nextData);
            if (gps.encode(nextData))
            {
                // Serial1.write("\n");

                if (gps.time.isUpdated() && gps.date.isUpdated())
                {
                    tm.tm_hour = gps.time.hour();
                    tm.tm_min = gps.time.minute();
                    tm.tm_sec = gps.time.second();
                    tm.tm_year = gps.date.year() - 1900;
                    tm.tm_mon = gps.date.month() - 1;
                    tm.tm_mday = gps.date.day();

                    timeReady = true;
                }
            }
        }
    }

    log_v("Date: %d-%d-%d\t Time : %d:%d:%d", gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second());

    return mktime(&tm); // return time and date from gps converted into timestamp
}

Position GNSS::parseRecord(struct Record *records)
{
    Position pos = {0};
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

    unsigned long lastRecord = 0;
    int idRecord = 0;

    while (millis() < start + TIME_GPS * 1000 && (!gpsOK || !timeOK) && depth < MAX_DEPTH_CHECK_GPS)
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
                if (year() > 1970 && !timeOK)
                {
                    timeOK = true;
                }
            }
            if (gps.location.isValid())
            {
                log_v("Position: %f , %f", getLat(), getLng());
                gpsOK = true;
                pos.Lat = (lat)gps.location.lat();
                pos.Lng = (lng)gps.location.lng();
            }
            depth = depthSensor.getDepth();
            if (millis() - lastRecord >= TIME_GPS_RECORDS)
            {
                lastRecord = millis();

                // save temp and depth
                temp = temperatureSensor.getTemp();
                records[idRecord].Depth = depth;
                records[idRecord].Temp = temp;
                records[idRecord].Time = (millis() - start) / 1000;
                idRecord++;
            }
        }
    }
    digitalWrite(GPIO_LED2, LOW);                              // turn syn led off when gps connected
    pos.dateTime = now() - idRecord * TIME_GPS_RECORDS / 1000; // start datetime is the before the gps search so we remove the duration of the gps search.
    log_d("DateTime: %ld\tNow:%ld", pos.dateTime, now());

    return pos;
}
