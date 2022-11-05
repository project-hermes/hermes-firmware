#include <Utils.hpp>

String remoraID()
{
    byte shaResult[32];
    WiFi.mode(WIFI_MODE_STA);
    String unhashed_id = WiFi.macAddress();
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    const size_t payloadLength = strlen(unhashed_id.c_str());

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)unhashed_id.c_str(), payloadLength);
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    String hash;
    for (int i = 0; i < sizeof(shaResult); i++)
    {
        char str[3];

        sprintf(str, "%02x", (int)shaResult[i]);
        hash = hash + str;
    }

    return hash;
}

void sleep(int mode)
{
    uint64_t wakeMask;
    switch (mode)
    {
        // if other mode, wake up with water, config, or charging
    case DEFAULT_SLEEP:
        log_i("DEFAULT SLEEP");
        pinMode(GPIO_PROBE, INPUT); // Set GPIO PROBE back to input to allow water detection
#ifndef MODE_DEBUG
        wakeMask = 1ULL << GPIO_WATER | 1ULL << GPIO_CONFIG | 1ULL << GPIO_VCC_SENSE;
#else
        wakeMask = 1ULL << GPIO_WATER | 1ULL << GPIO_CONFIG;
#endif
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);

        break;

        // if static diving, wake up with timer or config button
    case SLEEP_WITH_TIMER:
        log_i("SLEEP WITH TIMER");
        pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
        digitalWrite(GPIO_PROBE, LOW);
        gpio_hold_en(GPIO_NUM_33);
        gpio_deep_sleep_hold_en();

        wakeMask = 1ULL << GPIO_CONFIG;
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        esp_sleep_enable_timer_wakeup((TIME_TO_SLEEP_STATIC * 1000 - OFFSET_SLEEP_STATIC) * 1000);
        break;

        // if sd card error sleep without water detection
    case SDCARD_ERROR_SLEEP:
    case LOW_BATT_SLEEP:
        log_i("SD CARD ERROR OR LOW BATT SLEEP");
        pinMode(GPIO_PROBE, OUTPUT); // set gpio probe pin as low output to avoid corrosion
        digitalWrite(GPIO_PROBE, LOW);
        gpio_hold_en(GPIO_NUM_33);
        gpio_deep_sleep_hold_en();
#ifndef MODE_DEBUG
        wakeMask = 1ULL << GPIO_CONFIG | 1ULL << GPIO_VCC_SENSE;
#else
        wakeMask = 1ULL << GPIO_CONFIG;
#endif
        esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
        break;
    }

    log_i("Going to sleep now");
    esp_deep_sleep_start();
}

float readBattery()
{
    pinMode(GPIO_VBATT, INPUT_PULLUP);
    float val = analogRead(GPIO_VBATT);
    float vBat = ((val / 4095.0)) * 3.3 * 1.33;
    log_d("Batterie = %1.2f", vBat);
    pinMode(GPIO_VBATT, OUTPUT);

    return vBat;
}