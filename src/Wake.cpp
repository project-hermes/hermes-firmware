#include <Wake.hpp>

#define MODE_DEBUG false

using namespace std;
SecureDigital sd;

// variables permanentes pour le mode de plongée statique
RTC_DATA_ATTR Dive staticDive(&sd);
RTC_DATA_ATTR bool staticMode = false;
RTC_DATA_ATTR int staticCount;
RTC_DATA_ATTR long staticTime;

void wake()
{

    log_i("firmware version:%1.2f\n", FIRMWARE_VERSION);
    pinMode(GPIO_LED2, OUTPUT);
    pinMode(GPIO_LED3, OUTPUT);
    pinMode(GPIO_LED4, OUTPUT);
    pinMode(GPIO_WATER, INPUT);
    pinMode(GPIO_PROBE, INPUT);
    pinMode(GPIO_VBATT, INPUT);
    digitalWrite(GPIO_LED3, LOW);
    digitalWrite(GPIO_LED4, LOW);

    uint64_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        pinMode(GPIO_PROBE, INPUT);
        if (analogRead(GPIO_WATER) >= WATER_TRIGGER)
            staticCount = 0; // reset No water counter
        else
            staticCount++;           // if no water counter++
        pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
        digitalWrite(GPIO_PROBE, LOW);

        if (staticCount < maxStaticCounter)
        {
            recordStaticDive(); // new static record
            sleep(true);        // sleep with timer
        }
        else
        {
            endStaticDive();
            sleep(false); // sleep without timer waiting for other dive or config button
        }
    }
    else
    {

        wakeup_reason = esp_sleep_get_ext1_wakeup_status();

        uint64_t mask = 1;
        int i = 0;
        while (i < 64)
        {
            if (wakeup_reason & mask)
            {
                if (i == GPIO_WATER) // Start dive
                {
                    if (staticMode)
                    { // if Water wake up and staticMode
                        startStaticDive();
                        sleep(true);
                    }
                    else
                    {
                        dynamicDive();
                    }
                }
                else if (i == GPIO_VCC_SENSE) // wifi config
                {
                    startPortal(sd);
                }
                else if (i == GPIO_CONFIG) // button config (switch between diving modes)
                {
                    staticMode = !staticMode;

                    if (staticMode)
                    {
                        log_v("Static Diving");

                        digitalWrite(GPIO_LED4, HIGH);
                        delay(3000);
                        digitalWrite(GPIO_LED4, LOW);
                    }
                    else
                    {
                        log_v("Dynamic diving");

                        for (int i = 0; i < 10; i++)
                        {
                            digitalWrite(GPIO_LED4, HIGH);
                            delay(150);
                            digitalWrite(GPIO_LED4, LOW);
                            delay(150);
                        }
                    }
                }
            }

            i++;
            mask = mask << 1;
        }
    }
}

void sleep(bool timer)
{

    if (timer) // if static diving, wake up with timer or config button
    {
        pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
        digitalWrite(GPIO_PROBE, LOW);
        gpio_hold_en(GPIO_NUM_33);
        gpio_deep_sleep_hold_en();

        uint64_t wakeMask = 1ULL << GPIO_CONFIG;
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_STATIC * uS_TO_S_FACTOR);
    }
    else // if other mode, wake up with water, config, or charging
    {
#ifndef MODE_DEBUG
        uint64_t wakeMask = 1ULL << GPIO_WATER | 1ULL << GPIO_CONFIG | 1ULL << GPIO_VCC_SENSE;
#else
        uint64_t wakeMask = 1ULL << GPIO_WATER | 1ULL << GPIO_CONFIG;
#endif
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
    }

    log_i("Going to sleep now");
    esp_deep_sleep_start();
}

void dynamicDive()
{
    pinMode(GPIO_SENSOR_POWER, OUTPUT);
    digitalWrite(GPIO_SENSOR_POWER, LOW);
    delay(10);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);

    GNSS gps = GNSS();
    sd = SecureDigital();
    Dive d(&sd);
    tsys01 temperatureSensor = tsys01();
    ms5837 depthSensor = ms5837();

    bool led_on = false;

    if (d.Start(now(), gps.getLat(), gps.getLng(), TIME_DYNAMIC_MODE, staticMode) == "")
    {
        pinMode(GPIO_LED1, OUTPUT);
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(GPIO_LED1, HIGH);
            delay(300);
            digitalWrite(GPIO_LED1, LOW);
            delay(300);
        }
    }
    else
    {
        /* false while depth higher than minDepth */
        bool validDive = false;
        int count = 0;
        double depth, temp;
        long time = 0;

        while (count < maxCounter)
        {
            pinMode(GPIO_PROBE, INPUT);
            if (analogRead(GPIO_WATER) >= WATER_TRIGGER)
                count = 0; // reset No water counter
            else
                count++; // if no water counter++

            pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
            digitalWrite(GPIO_PROBE, LOW);

            temp = temperatureSensor.getTemp();
            depth = depthSensor.getDepth();
            time += (TIME_DYNAMIC_MODE / 1000); // get time in seconds since wake up

            if (validDive == false) // if dive still not valid, check if depthMin reached
            {
                if (depth > minDepth)
                    validDive = true; // if minDepth reached, dive is valid
            }

            Record tempRecord = Record{temp, depth, time};
            d.NewRecord(tempRecord);

            delay(TIME_DYNAMIC_MODE);
            if (led_on)
            {
                digitalWrite(GPIO_LED4, HIGH);
            }
            else
            {
                digitalWrite(GPIO_LED4, LOW);
            }
            led_on = !led_on;
        }

        String ID = d.End(now(), gps.getLat(), gps.getLng());

        if (ID == "")
        {
            log_e("error ending the dive");
        }
        else
        {
            if (!validDive)
            {
                d.deleteID(ID);
                log_v("Dive not valid, record deleted");
            }
        }
    }
}

void startStaticDive()
{
    pinMode(GPIO_SENSOR_POWER, OUTPUT);
    digitalWrite(GPIO_SENSOR_POWER, LOW);
    delay(10);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);

    GNSS gps = GNSS();
    sd = SecureDigital();
    tsys01 temperatureSensor = tsys01();
    ms5837 depthSensor = ms5837();

    staticCount = 0;

    if (staticDive.Start(now(), gps.getLat(), gps.getLng(), TIME_TO_SLEEP_STATIC, staticMode) == "")
    {
        pinMode(GPIO_LED1, OUTPUT);
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(GPIO_LED1, HIGH);
            delay(300);
            digitalWrite(GPIO_LED1, LOW);
            delay(300);
        }
    }
    else
    {
        pinMode(GPIO_LED4, OUTPUT);
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(GPIO_LED4, HIGH);
            delay(300);
            digitalWrite(GPIO_LED4, LOW);
            delay(300);
        }
        double depth, temp;
        staticTime = 0;

        temp = temperatureSensor.getTemp();
        depth = depthSensor.getDepth();

        Record tempRecord = Record{temp, depth, staticTime};
        staticDive.NewRecordStatic(tempRecord);
    }
}

void recordStaticDive()
{
    pinMode(GPIO_SENSOR_POWER, OUTPUT);
    digitalWrite(GPIO_SENSOR_POWER, LOW);
    delay(10);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);

    tsys01 temperatureSensor = tsys01();
    ms5837 depthSensor = ms5837();
    double depth, temp;

    temp = temperatureSensor.getTemp();
    depth = depthSensor.getDepth();
    staticTime += TIME_TO_SLEEP_STATIC;

    Record tempRecord = Record{temp, depth, staticTime};
    staticDive.NewRecordStatic(tempRecord);
}

void endStaticDive()
{
    pinMode(GPIO_SENSOR_POWER, OUTPUT);
    digitalWrite(GPIO_SENSOR_POWER, LOW);
    delay(10);
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(10);
    GNSS gps = GNSS();
    tsys01 temperatureSensor = tsys01();
    ms5837 depthSensor = ms5837();
    double depth, temp;

    temp = temperatureSensor.getTemp();
    depth = depthSensor.getDepth();
    staticTime += TIME_TO_SLEEP_STATIC;

    Record tempRecord = Record{temp, depth, staticTime};
    staticDive.NewRecordStatic(tempRecord);
    String ID = staticDive.End(now(), gps.getLat(), gps.getLng());

    if (ID == "")
    {
        log_e("error ending the dive");
    }
}
