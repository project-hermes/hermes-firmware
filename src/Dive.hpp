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

private:
    Storage *storage;
    String ID;
    DiveMetadata metadata;
    int order = 0;
    const String recordSchema[2] = {"temp", "depth"};
    const int siloRecordSize = 300;
    const int siloByteSize = 27000;
    int currentRecords = 0;
    Record *diveRecords;

    int writeSilo()
    {
        //FYI if this is not big enough it will just cut off what can't fit
        DynamicJsonDocument jsonSilo(siloByteSize);

        jsonSilo["id"] = ID;
        jsonSilo["order"] = order;
        order++;

        JsonArray records = jsonSilo.createNestedArray("records");
        for (int i = 0; i < siloRecordSize; i++)
        {
            JsonArray record = records.createNestedArray();
            record.add(diveRecords[i].Temp);
            record.add(diveRecords[i].Depth);
        }

        String buffer;
        serializeJson(jsonSilo, buffer);
        
        return storage->writeFile(String("/" + ID + "/silos_" + order + ".json").c_str(), buffer);
    }

    int writeMetadataStart(long time, double lat, double lng, int freq)
    {
        String data;

        storage->makeDirectory(String("/" + ID).c_str());

        data = data + "deviceId:" + remoraID() + "\n";
        data = data + "diveId:" + ID + "\n";
        data = data + "startTime:" + time + "\n";
        data = data + "startLat:" + lat + "\n";
        data = data + "startLng:" + lng + "\n";
        data = data + "freq:" + freq + "\n";
        data = data + "schema:[";
        for (int i = 0; i < 2; i++)
        {
            data = data + "," + recordSchema[i];
        }
        data = data + "]\n";

        return storage->writeFile(String("/" + ID + "/metadata.txt").c_str(), data.c_str());
    }

    int writeMetadataEnd(long time, double lat, double lng)
    {
        String data;

        data = data + "endTime:" + time + "\n";
        data = data + "endLat:" + lat + "\n";
        data = data + "endLng:" + lng + "\n";
        data = data + "numberOfSilos:" + order + "\n";

        return storage->appendFile(String("/" + ID + "/metadata.txt").c_str(), data.c_str());
    }

    String createID(long time)
    {
        byte shaResult[32];
        WiFi.mode(WIFI_MODE_STA);
        String unhashed_id = String(time) + WiFi.macAddress();
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
};