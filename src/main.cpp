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

WebServer Server;
AutoConnect Portal(Server);


String getMacAddress() {
	uint8_t baseMac[6];
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	char baseMacChr[18] = {0};
	sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
	return String(baseMacChr);
}

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
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial.println();

  pinMode(12, INPUT);

  Serial.printf("system starting...\n");
  Serial.printf("build Version: %f\n", BUILD_VERSION);
  Serial.printf("built At: %d\n", BUILD_TIME);
  Serial.printf("uuid: %s\n", getMacAddress().c_str());
  delay(100);

  wakeup();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 1);
  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
}

void loop() {}