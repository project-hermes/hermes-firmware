#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>
#include <AutoConnect.h>

#include <Dive.hpp>
#include <hal/TSYS01.hpp>
#include <hal/MS5837.hpp>
#include <hal/remora-hal.h>
#include <Storage/SecureDigital.hpp>
#include <Navigation/GNSS.hpp>
#include <Utils.hpp>
#include <Wake.hpp>

// this is so bad, I know
#define FIRMWARE_VERSION 2

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) between each static record */

#define minDepth 0 /* Min depth to validate dynamic dive */
#define maxCounter 30 /* Number of No Water to end dive */

using namespace std;
// TODO remove
SecureDigital sd;

// variables permanentes pour le mode de plongée statique
RTC_DATA_ATTR Dive staticDive(&sd);
RTC_DATA_ATTR bool staticDiving = false;

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
        pinMode(GPIO_LED2, OUTPUT);
        digitalWrite(GPIO_LED2, HIGH);

        // Static diving, timer wake up
        Serial.println("Static diving, timer wake up");
        tsys01 temperatureSensor = tsys01();
        ms5837 depthSensor = ms5837();
        pinMode(GPIO_WATER, OUTPUT);
        Record tempRecord = Record{temperatureSensor.getTemp(), depthSensor.getDepth()};
        Serial.print("Temperature = "), Serial.println(tempRecord.Temp / 1000000000);
        Serial.print("Depth = "), Serial.println(tempRecord.Depth);

        staticDive.NewRecord(tempRecord);
        digitalWrite(GPIO_LED2, LOW);
    }
    else
    {
        wakeup_reason = esp_sleep_get_ext1_wakeup_status();

        uint64_t mask = 1;
        int i = 0;
        bool led_on = false;
        while (i < 64)
        {
            if (wakeup_reason & mask)
            {
                Serial.printf("Wakeup because %d\n", i);
                if (i == GPIO_WATER) // Start dive
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
                                count++; //if no water counter++

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

                        if (d.End(now(), gps.getLat(), gps.getLng()) == "")
                        {
                            Serial.println("error ending the dive");
                        }
                    }

                    Serial.println("done");
                }
                else if (i == GPIO_VCC_SENSE) // wifi config
                {
                    startPortal();
                }
                else if (i == GPIO_CONFIG) // button config
                {

                    if (!staticDiving)
                    {
                        pinMode(GPIO_LED2, OUTPUT);
                        for (int i = 0; i < 3; i++)
                        {
                            digitalWrite(GPIO_LED2, HIGH);
                            delay(300);
                            digitalWrite(GPIO_LED2, LOW);
                            delay(300);
                        }
                        staticDiving = true;
                        Serial.println("Start Static Diving");
                        pinMode(GPIO_SENSOR_POWER, OUTPUT);
                        digitalWrite(GPIO_SENSOR_POWER, LOW);
                        delay(10);
                        Wire.begin(I2C_SDA, I2C_SCL);
                        delay(10);

                        GNSS gps = GNSS();
                        sd = SecureDigital();

                        String diveId = staticDive.Start(now(), gps.getLat(), gps.getLng());
                        Serial.print("Dive ID = "), Serial.println(diveId);
                    }
                    else
                    {
                        staticDiving = false;
                        Serial.println("Stop static Diving ");
                    }
                }
            }

            i++;
            mask = mask << 1;
        }
    }
}

void sleep()
{
    if (staticDiving) // if static diving, wake up with timer or config button
    {
        uint64_t wakeMask = 1ULL << GPIO_CONFIG;
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    }
    else // if other mode, wake up with water, config, or charging
    {
        uint64_t wakeMask = 1ULL << GPIO_WATER | 1ULL << GPIO_CONFIG | 1ULL << GPIO_VCC_SENSE;
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
    }
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();
}

void startPortal()
{
    WebServer Server;
    AutoConnect Portal(Server);

    Serial.printf("starting config portal...\n");
    AutoConnectConfig acConfig("Remora Config", "cousteau");
    acConfig.hostName = "remora";
    acConfig.homeUri = "/remora";
    acConfig.autoReconnect = true;
    acConfig.autoReset = false;
    acConfig.portalTimeout = 15 * 60 * 1000;
    acConfig.title = "Remora Config";
    acConfig.ticker = true;
    acConfig.tickerPort = GPIO_LED3;
    acConfig.tickerOn = LOW;
    Portal.config(acConfig);
    Portal.begin();

    while (WiFi.status() == WL_DISCONNECTED)
    {
        Portal.handleClient();
    }
    Serial.println(WiFi.localIP());

    Serial.println("Wifi connected, start upload dives");
    uploadDives();
    // Serial.println("disconnecting");
    // cloud.disconnect();
    return;

    // turned off for testing
    /*if (digitalRead(GPIO_VCC_SENSE) == 1)
    {
        while (WiFi.status() == WL_DISCONNECTED)
        {
            Portal.handleClient();
            if (digitalRead(GPIO_VCC_SENSE) == 0)
            {
                return;
            }
        }
        ota();
        sendJson();
    }
    while (digitalRead(GPIO_VCC_SENSE) == 1)
    {
        Portal.handleClient();
    }*/
}

int uploadDives()
{

    GoogleCloudIOT cloud = GoogleCloudIOT();
    cloud.connect();

    StaticJsonDocument<1024> indexJson;
    sd = SecureDigital();

    String data;
    data = sd.readFile("/index.json");
    if (data == "")
    {
        Serial.println("Could not read index file to upload dives");
        return -1;
    }
    deserializeJson(indexJson, data);
    log_v("\nNb dives recorded = %d\n");
    for (int i = 0; i < indexJson.size(); i++)
    {
        JsonObject dive = indexJson[i].as<JsonObject>();
        if (dive["uploadedAt"] != 0)
        {
            continue;
        }
        StaticJsonDocument<512> diveMetadataJson;
        String diveId = dive["id"];
        String diveMetadata = sd.readFile(String("/" + diveId + "/metadata.json"));
        if (diveMetadata == "")
        {
            Serial.println("could not load dive metadata for dive " + diveId);
            return -1;
        }

        deserializeJson(diveMetadataJson, diveMetadata);

        int numberOfSilos = diveMetadataJson["numberOfSilos"];

        for (int i = 1; i < numberOfSilos + 1; i++)
        {
            String diveSilo = sd.readFile(String("/" + diveId + "/" + "silo_" + i + ".json"));
            if (diveSilo == "")
            {
                Serial.println("could not load dive silo " + String(i) + " for dive " + diveId);
                return -1;
            }
            else
            {
                Serial.println("Loaded silo " + String(i) + " of dive " + diveId);
            }

            if (cloud.upload("Test dive data") == -1)
            {
                Serial.println("Could not upload silo " + String(i) + " of dive " + diveId);
                return -1;
            }
            else
            {
                Serial.println("Uploaded silo " + String(i) + " of dive " + diveId);
            }
        }
        // TODO update the index so the dive does not reuplaod
    }
    return 0;
}

void ota()
{
    String cloudFunction = "http://us-central1-project-hermes-staging.cloudfunctions.net/ota";
    String bucketURL = "http://storage.googleapis.com/remora-firmware/";
    String version;
    HTTPClient http;

    Serial.println(WiFi.localIP());

    if (http.begin(cloudFunction))
    {
        if (http.GET() == 200)
        {
            version = http.getString();
            if (version.toInt() <= FIRMWARE_VERSION)
            {
                http.end();
                Serial.printf("Will not update as I am version:%ld and you are offering version:%d\n", version.toInt(), FIRMWARE_VERSION);
                return;
            }
        }
        else
        {
            Serial.println("could not contact cloud function");
            http.end();
            return;
        }
    }
    else
    {
        Serial.println("could not begin http client");
    }
    http.end();

    size_t written = 0;
    size_t gotten = 1;
    String firmware_url = bucketURL + "firmware_" + FIRMWARE_VERSION + ".bin";
    if (http.begin(firmware_url))
    {
        if (http.GET() == 200)
        {
            gotten = http.getSize();
            if (!Update.begin(gotten))
            {
                Serial.printf("Firmware file too big at %d\n", gotten);
                http.end();
                return;
            }
            Serial.println("atempting to update...");
            written = Update.writeStream(http.getStream());
        }
        http.end();
    }
    else
    {
        Serial.println("could not get update file");
        http.end();
        return;
    }

    if (written == gotten)
    {
        Serial.println("Written : " + String(written) + " successfully");
    }
    else
    {
        Serial.println("Written only : " + String(written) + "/" + String(gotten) + ". Retry?");
    }

    if (Update.end())
    {
        Serial.println("OTA done!");
        if (Update.isFinished())
        {
            Serial.println("Update successfully completed. Rebooting.");
            ESP.restart();
        }
        else
        {
            Serial.println("Update not finished? Something went wrong!");
        }
    }
    else
    {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }
}