#include <Arduino.h>
#include <ArduinoLog.h>
#include <AutoConnect.h>
#include <WiFi.h>
#include <WebServer.h>

#ifdef DEBUG
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#define AC_DEBUG
#define AC_DEBUG_PORT Serial
#endif

#ifndef DEBUG
#define LOG_LEVEL LOG_LEVEL_NOTICE
#undef AC_DEBUG
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION 0.0
#endif

#define CONFIG_PIN 27

enum Mode
{
  Sleeping,
  Diving,
  Config
};

WebServer Server;
AutoConnect Portal(Server);

Mode mode = Sleeping;

void printTimestamp(Print *_logOutput)
{
  char c[12];
  sprintf(c, "%10lu ", millis());
  _logOutput->print(c);
}

void checkButtons()
{
  if (digitalRead(CONFIG_PIN) == HIGH && mode == Sleeping)
  {
    mode = Config;
    Log.notice("starting config portal\n");
    AutoConnectConfig acConfig(AP_NAME, AP_PASS);
    acConfig.autoReconnect = false;
    acConfig.autoReset = true;
    Portal.config(acConfig);
    Portal.begin();
  }
}

void runPortal()
{
  if (mode == Config && WiFi.getMode() == WIFI_AP_STA)
  {
    Portal.handleClient();
  }
}
void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial.println();

  pinMode(CONFIG_PIN, INPUT);

  Log.begin(LOG_LEVEL, &Serial);
  Log.setPrefix(printTimestamp);

  Log.notice("system starting...\n");
  Log.notice("build Version: %D\n", BUILD_VERSION);
  Log.notice("built At: %d\n", BUILD_TIME);
}

void loop()
{
  checkButtons();
  runPortal();
}