#include <Wake.hpp>

using namespace std;
SecureDigital sd;

// variables permanentes pour le mode de plongée statique
RTC_DATA_ATTR Dive staticDive(&sd);
RTC_DATA_ATTR bool staticMode = false;

void wake()
{
    Serial.printf("firmware version:%d\n", FIRMWARE_VERSION);

    pinMode(GPIO_LED2, OUTPUT);
    pinMode(GPIO_LED3, OUTPUT);
    pinMode(GPIO_LED4, OUTPUT);
    pinMode(GPIO_WATER, INPUT);
    pinMode(GPIO_VBATT, INPUT);
    digitalWrite(GPIO_LED3, LOW);
    digitalWrite(GPIO_LED4, LOW);

    uint64_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.print("Wake Up reason = "), Serial.println(wakeup_reason);

    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        // new record

        sleep(true);
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
                Serial.printf("Wakeup because %d\n", i);
                if (i == GPIO_WATER) // Start dive
                {
                    if (staticMode)
                    { // if Water wake up and staticMode
                        startStaticDive();
                    }
                    else
                    {
                        dynamicDive();
                    }
                    Serial.println("done");
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
                        Serial.println("Static Diving");

                        digitalWrite(GPIO_LED4, HIGH);
                        delay(3000);
                        digitalWrite(GPIO_LED4, LOW);
                    }
                    else
                    {
                        Serial.println("Dynamic diving");

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
        uint64_t wakeMask = 1ULL << GPIO_CONFIG;
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    }
    else // if other mode, wake up with water, config, or charging
    {
        uint64_t wakeMask = 1ULL << GPIO_WATER | 1ULL << GPIO_CONFIG /*| 1ULL << GPIO_VCC_SENSE*/;
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
    }

    Serial.println("Going to sleep now");
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

    if (d.Start(now(), gps.getLat(), gps.getLng()) == "")
    {
        Serial.println("error starting the dive");
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
        /* Test water sensor */
        while (1)
        {
            pinMode(GPIO_WATER, OUTPUT);

            Serial.print("Water = "), Serial.println(digitalRead(GPIO_WATER));
            delay(500);
        }
        /* End Test water sensor */

        while (count < maxCounter)
        {
            if (digitalRead(GPIO_WATER) == 1)
                count = 0; // reset No water counter
            else
                count++; // if no water counter++

            pinMode(GPIO_WATER, OUTPUT);

            temp = temperatureSensor.getTemp();
            depth = depthSensor.getDepth();

            if (validDive == false) // if dive still not valid, check if depthMin reached
            {
                if (depth < minDepth)
                    validDive = true; // if minDepth reached, dive is valid
            }

            Record tempRecord = Record{temp, depth};
            d.NewRecord(tempRecord);

            delay(1000);
            if (led_on)
            {
                digitalWrite(GPIO_LED2, HIGH);
            }
            else
            {
                digitalWrite(GPIO_LED2, LOW);
            }
            pinMode(GPIO_WATER, INPUT);
            led_on = !led_on;
        }

        String ID = d.End(now(), gps.getLat(), gps.getLng());

        if (ID == "")
        {
            Serial.println("error ending the dive");
        }
        else
        {
            if (!validDive)
            {
                d.deleteID(ID);
                Serial.println("Dive not valid, record deleted");
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

    if (staticDive.Start(now(), gps.getLat(), gps.getLng()) == "")
    {
        Serial.println("error starting the static dive");
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
        double depth, temp;

        temp = temperatureSensor.getTemp();
        depth = depthSensor.getDepth();

        Record tempRecord = Record{temp, depth};
        staticDive.NewRecord(tempRecord);
    }

    sleep(true);
}
