#ifndef DIVE_HPP
#define DIVE_HPP

#include <mbedtls/md.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include <Storage/Storage.hpp>
#include <Types.hpp>
#include <Utils.hpp>

using namespace std;

struct DiveMetadata
{
    String ID;
    long startTime;
    long endTime;
    int freq;
    int numSilos;
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
};

class Dive
{
public:
    Dive();
    Dive(Storage *s);
    String Start(long time, lat lat, lng lng);
    String End(long time, lat lat, lng lng);
    int NewRecord(Record r);
    void deleteID(String ID);

private:
    Storage *storage;
    String ID;
    DiveMetadata metadata;
    int order = 0;
    const String recordSchema[2] = {"temp", "depth"};
    const int siloRecordSize = 300;
    const int siloByteSize = 27000;
    const int indexByteSize = 27000;
    const String indexPath = "/index.json";
    int currentRecords = 0;
    Record *diveRecords;

    int writeSilo();
    int writeMetadataStart(long time, double lat, double lng, int freq);
    int writeMetadataEnd(long time, double lat, double lng);
    int updateIndex();
    String createID(long time);
};

#endif