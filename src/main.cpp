#include <Arduino.h>
#include <AutoConnect.h>
#include <WiFi.h>
#include <WebServer.h>
#include "dive.pb.h"
#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include <HTTPClient.h>

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

String getMacAddress()
{
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
  acConfig.autoReconnect = true;
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

  //wakeup();

  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_12, 1);
  //Serial.println("Going to sleep now");
  //esp_deep_sleep_start();
  //runPortal();
  WiFi.begin("COS", "thexfiles"); 
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) { //Check for the connection
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to the WiFi network");
}

void loop()
{
  //delay(2000);
  uint8_t buffer[128];
  types_Dive testDive = types_Dive_init_zero;
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  testDive.sensorId = 123;
  bool status = pb_encode(&stream, types_Dive_fields, &testDive);
  if (!status)
  {
    Serial.println("Failed to encode");
    return;
  }
  Serial.print("Message Length: ");
  Serial.println(stream.bytes_written);

  Serial.print("Message: ");

  for (int i = 0; i < stream.bytes_written; i++)
  {
    Serial.printf("%02X", buffer[i]);
  }
  Serial.println();

  HTTPClient http;
  http.begin("http://project-hermes-staging.appspot.com/dive");
  http.addHeader("Content-Type", "application/protobuf");
  int code = http.POST((const char*)buffer);
  if (code > 0)
  {
    String response = http.getString(); //Get the response to the request
    Serial.println(code); //Print return code
    Serial.println(response);         //Print request answer
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(code);
  }
  http.end();
}