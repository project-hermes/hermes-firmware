#include <Arduino.h>
#include <AutoConnect.h>
#include <WiFi.h>
#include <WebServer.h>

#ifdef DEBUG
#define AC_DEBUG
#define AC_DEBUG_PORT Serial
#endif

#ifndef DEBUG
#undef AC_DEBUG
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION 0.0
#endif

#define CONFIG_PIN 27

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

WebServer Server;
AutoConnect Portal(Server);


void runPortal()
{
  Serial.printf("starting config portal\n");
  AutoConnectConfig acConfig(AP_NAME, AP_PASS);
  acConfig.autoReconnect = false;
  acConfig.autoReset = true;
  Portal.config(acConfig);
  Portal.begin();
  while (WiFi.getMode() == WIFI_AP_STA)
  {
    Portal.handleClient();
  }
}

void wakeup()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    runPortal();
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    delay(1000);
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    delay(1000);
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    delay(1000);
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    delay(1000);
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    delay(1000);
    break;
  }
}

void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial.println();

  pinMode(CONFIG_PIN, INPUT);

  Serial.printf("system starting...\n");
  Serial.printf("build Version: %D\n", BUILD_VERSION);
  Serial.printf("built At: %d\n", BUILD_TIME);

  wakeup();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 1);
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

void loop() {}