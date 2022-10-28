#include <Wake.hpp>

// #define MODE_DEBUG

using namespace std;
SecureDigital sd;

// variables permanentes pour le mode de plongée statique
RTC_DATA_ATTR Dive staticDive(&sd);
RTC_DATA_ATTR bool diveMode = 0; //0:dynamic, 1:static
RTC_DATA_ATTR int staticCount;
RTC_DATA_ATTR long staticTime;

/// @brief Interrupt routine to shutdown remora if wifi is disconnected
/// @return
void IRAM_ATTR ISR()
{
    sleep(false);
}

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

    pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
    digitalWrite(GPIO_PROBE, LOW);

    uint64_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        log_d("Wake up timer static");
        gpio_hold_dis(GPIO_NUM_33);
        staticDiveWakeUp();
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
                    log_d("Wake up gpio water");
                    if (diveMode == STATIC_MODE)
                    { // if Water wake up and static Mode
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
                    log_d("Wake up gpio vcc sense");

                    // While wifi not set, shutdown if usb is disconnected
                    attachInterrupt(GPIO_VCC_SENSE, ISR, FALLING);

                    startPortal(sd);
                }
                else if (i == GPIO_CONFIG) // button config (switch between diving modes)
                {
                    log_d("Wake up gpio config");
                    selectMode();
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
        esp_sleep_enable_timer_wakeup((TIME_TO_SLEEP_STATIC * 1000 - OFFSET_SLEEP_STATIC) * 1000);
    }
    else // if other mode, wake up with water, config, or charging
    {
        pinMode(GPIO_PROBE, INPUT); // Set GPIO PROBE back to input to allow water detection
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
    pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
    digitalWrite(GPIO_PROBE, LOW);

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

    if (d.Start(now(), gps.getLat(), gps.getLng(), TIME_DYNAMIC_MODE, diveMode) == "")
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
        unsigned long startTime = millis(), previous = millis();

        // if valid dive, dive end after short time, if dive still not valid, dive end after long time
        while (count < validDive ? MAX_DYNAMIC_COUNTER_VALID_DIVE : MAX_DYNAMIC_COUNTER_NO_DIVE)
        {
            if (millis() - previous > TIME_DYNAMIC_MODE)
            {
                previous = millis();
                time = (millis() - startTime) / 1000; // get time in seconds since wake up

                temp = temperatureSensor.getTemp();
                depth = depthSensor.getDepth();

                // if dive still not valid, check if depthMin reached
                if (validDive == false)
                {
                    if (depth > MIN_DEPTH_VALID_DIVE)
                        validDive = true; // if minDepth reached, dive is valid
                }

                // check water only if depth < MAX DEPTH CHECK WATER
                if (depth < MAX_DEPTH_CHECK_WATER)
                {
                    pinMode(GPIO_PROBE, INPUT); // enable probe pin to allow water detection
                    int value = analogRead(GPIO_WATER);
                    log_d("Value = %d", value);
                    if (value >= WATER_TRIGGER)
                        count = 0; // reset No water counter
                    else
                        count++;                 // if no water counter++
                    pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
                    digitalWrite(GPIO_PROBE, LOW);
                }
                Record tempRecord = Record{temp, depth, time};
                d.NewRecord(tempRecord);

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
        }

        if (validDive)
        {
            //if dive valid (Pmin reached) get end GPS and 
            String end = d.End(now(), gps.getLat(), gps.getLng(), diveMode);

            if (end == "")
            {
                log_e("error ending the dive");
            }
        }
        else
        {
            // if dive not valid (Pmin not reached), delete records and clean index
            d.deleteID(d.getID());
            log_v("Dive not valid, record deleted");
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

    if (staticDive.Start(now(), gps.getLat(), gps.getLng(), TIME_TO_SLEEP_STATIC, diveMode) == "")
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

void staticDiveWakeUp()
{
    pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
    digitalWrite(GPIO_PROBE, LOW);
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

    if (depth < MAX_DEPTH_CHECK_WATER)
    {
        pinMode(GPIO_PROBE, INPUT); // enable probe pin to allow water detection
        int value;

        value = analogRead(GPIO_WATER);
        log_d("Value = %d", value);

        if (value >= WATER_TRIGGER)
            staticCount = 0; // reset No water counter
        else
            staticCount++;           // if no water counter++
        pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
        digitalWrite(GPIO_PROBE, LOW);
    }

    Record tempRecord = Record{temp, depth, staticTime};
    staticDive.NewRecordStatic(tempRecord);

    if (staticCount < MAX_STATIC_COUNTER)
    {
        sleep(true); // sleep with timer
    }
    else
    {
        GNSS gps = GNSS();

        String ID = staticDive.End(now(), gps.getLat(), gps.getLng(), diveMode);

        if (ID == "")
        {
            log_e("error ending the dive");
        }
        sleep(false); // sleep without timer waiting for other dive or config button
    }
}

void selectMode()
{
    diveMode = !diveMode;

    if (diveMode==STATIC_MODE)
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