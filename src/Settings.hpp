#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#define DEFAULT_SLEEP 0
#define SLEEP_WITH_TIMER 1
#define SDCARD_ERROR_SLEEP 2
#define LOW_BATT_SLEEP 3

#define FIRMWARE_VERSION 3.6

// Debug parameters
// #define MODE_DEBUG
// #define SERIAL1_DEBUG_OUTPUT

// Dives parameters
#define OFFSET_SLEEP_STATIC 1700         /*Offset going to sleep and wake up static mode*/
#define TIME_TO_SLEEP_STATIC 10          /* Time ESP32 will go to sleep (in seconds) between each static record */
#define TIME_DYNAMIC_MODE 1              /* Time between two records in dynamic mode (s)*/
#define TIME_GPS_RECORDS 5               /* Time between records during gps search*/
#define MIN_DEPTH_VALID_DIVE 5           /* Min depth to validate dynamic dive (meters)*/
#define MAX_DEPTH_CHECK_WATER 0.5        /* Max depth for water detection (meter)*/
#define MAX_DEPTH_CHECK_GPS 1.0          /* Max depth for gps detection (meter)*/
#define MAX_DYNAMIC_COUNTER_VALID_DIVE 1 /* Number of No Water to end dynamic dive (TIME_DYNAMIC_MODE / 1000 * MAX_DYNAMIC_COUNTER = seconds)*/
#define MAX_DYNAMIC_COUNTER_NO_DIVE 600  /* Number of No Water to end dynamic dive (TIME_DYNAMIC_MODE / 1000 * MAX_DYNAMIC_COUNTER = seconds)*/
#define MAX_STATIC_COUNTER 2             /* Number of No Water to end static dive (TIME_TO_SLEEP_STATIC * MAX_STATIC_COUNTER = seconds )*/
#define WATER_TRIGGER 1500               // mV water detection level (0 to 3300) (lower value for pure water)
#define TIME_GPS_START 600               // research time gps at the beginning of the dive(seconds)
#define TIME_GPS_END 900                 // research time gps at the end of the dive (seconds)
#define TIME_SURFACE_DETECTION 5         // Time to detect surface crossing at the beginning of the dive (seconds)
#define BEGIN_SURFACE_DETECTION 0.05     // depth max-min to detect surface crossing at the beggining of the dive
#define TIME_CHECK_BATTERY 60 // Time between 2 battery ccheck during dynamic dive. (seconds)
#define LOW_BATTERY_LEVEL 3.3 //  If vBat < Low battery level, go back to sleep without water detection wakeup (Volts)
#define TIME_UPLOAD_OTA 1800             // Time between 2 upload and OTA check (seconds)
#define MIN_DEPTH_CHECK_AMPLITUDE 2.0    // min depth to check amplitude of depth before ending dive. (meter)
#define ENDING_DIVE_DEPTH_AMPLITUDE 0.08 // min depth amplitude to end dive, if depth amplitude below val, diver is out of water.

// Upload parameters
const String indexPath = "/index.json";
#define POST_RETRY 3 // number of post attemp before skip.

#endif
