#include <Arduino.h>
#include <ArduinoLog.h>

#ifdef DEBUG
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#endif
#ifndef DEBUG
#define LOG_LEVEL LOG_LEVEL_NOTICE
#endif
#ifndef BUILD_VERSION
#define BUILD_VERSION 0.0
#endif

void printTimestamp(Print* _logOutput) {
  char c[12];
  sprintf(c, "%10lu ", millis());
  _logOutput->print(c);
}

void setup()
{
  Serial.begin(9600);
  delay(100);
  Serial.println();

  Log.begin(LOG_LEVEL, &Serial);
  Log.setPrefix(printTimestamp);

  Log.notice("System starting...\n");
  Log.notice("Build Version: %D\n",BUILD_VERSION);
  Log.notice("Built At: %d\n",BUILD_TIME);
}

void loop()
{
}