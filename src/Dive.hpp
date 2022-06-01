#ifndef DIVE_HPP
#define DIVE_HPP

#include <mbedtls/md.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <AutoConnect.h>

#include <Storage/Storage.hpp>
#include <Types.hpp>
#include <Utils.hpp>
#include <Connect.hpp>

using namespace std;

struct DiveMetadata
{
    String ID;
    long startTime;
    long endTime;
    int freq;
    int siloSize;
    float startLat;
    float startLng;
    float endLat;
    float endLng;
};

struct Record
{
    temperature Temp;
    depth Depth;
    long Time;
};

class Dive
{
public:
    Dive();
    Dive(Storage *s);

    String Start(long time, lat lat, lng lng, int freq, bool mode);
    String End(long time, lat lat, lng lng);

    int NewRecord(Record r);
    int NewRecordStatic(Record r);

    void saveId(String ID);
    void deleteID(String ID);
    
    void sendJson();

private:
    Storage *storage;
    String ID;
    DiveMetadata metadata;
    int currentRecords = 0;
    Record *diveRecords;
    const int siloRecordSize = 5;
    const int siloByteSize = 27000;
    const int indexByteSize = 27000;
    int order = 0;

    void init();

    int createIndex();
    int updateIndex(String updatedID);
    int deleteIndex(String deletedID);

    String createID(long time);
    String getID();

    int writeMetadataEnd(long time, double lat, double lng);
    int writeMetadataStart(long time, double lat, double lng, int freq, bool mode);
    int writeSilo();
    int writeStaticRecord();
};

#endif