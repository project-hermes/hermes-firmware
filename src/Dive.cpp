#include <Dive.hpp>
#include <Storage/Storage.hpp>

Dive::Dive(void) {}

Dive::Dive(Storage *s)
{
    storage = s;
}

void Dive::init()
{

    ID = "";
    order = 0;
    currentRecords = 0;
    Record *diveRecords;
    metadata.ID = "";
    metadata.startTime = 0;
    metadata.endTime = 0;
    metadata.freq = 0;
    metadata.numSilos = 0;
    metadata.siloSize = 0;
    metadata.startLat = 0;
    metadata.startLng = 0;
    metadata.endLat = 0;
    metadata.endLng = 0;
    diveRecords->Temp = 0;
    diveRecords->Depth = 0;
}

String Dive::Start(long time, lat lat, lng lng)
{
    init();
    Serial.println(time);
    ID = createID(time);
    diveRecords = new Record[siloRecordSize];
    if (writeMetadataStart(time, lat, lng, 1) == -1)
    {
        return "";
    }
    return ID;
}

String Dive::End(long time, lat lat, lng lng)
{
    Serial.println(time);
    if (writeMetadataEnd(time, lat, lng) == -1)
    {
        return "";
    }
    return ID;
}

int Dive::NewRecord(Record r)
{
    diveRecords[currentRecords] = r;
    currentRecords++;
    if (currentRecords == siloRecordSize)
    {
        if (writeSilo() == -1)
        {
            Serial.println("error saving silo");
            return -1;
        }
        delete[] diveRecords;
        diveRecords = new Record[siloRecordSize];
        currentRecords = 0;
    }
    return 0;
}

int Dive::writeSilo()
{

    String path = "/" + ID + ".json";

    // this should only happen to a new dive record
    if (storage->findFile(path) == -1)
    {

        DynamicJsonDocument jsonSilo(siloByteSize);

        jsonSilo["id"] = ID;

        JsonArray records = jsonSilo.createNestedArray("records");
        for (int i = 0; i < siloRecordSize; i++)
        {
            JsonArray record = records.createNestedArray();
            record.add(diveRecords[i].Temp);
            record.add(diveRecords[i].Depth);
        }

        String buffer;
        serializeJson(jsonSilo, buffer);

        return storage->writeFile(path, buffer);
    }
    else
    {
        String recordsID = storage->readFile(path);
        if (recordsID == "")
        {
            Serial.println("Could not read previous ID records file");
            return -1;
        }
        else
        {
            DynamicJsonDocument newRecordsID(siloByteSize);
            deserializeJson(newRecordsID, recordsID);
            JsonArray dives = newRecordsID.to<JsonArray>();

            JsonArray records = newRecordsID.createNestedArray("records");
            for (int i = 0; i < siloRecordSize; i++)
            {
                JsonArray record = records.createNestedArray();
                record.add(diveRecords[i].Temp);
                record.add(diveRecords[i].Depth);
            }

            String buffer;
            serializeJson(newRecordsID, buffer);

            return storage->writeFile(path, buffer);
        }
    }
}

int Dive::writeMetadataStart(long time, double lat, double lng, int freq)
{
    StaticJsonDocument<1024> mdata;

    storage->makeDirectory(String("/" + ID).c_str());

    mdata["deviceId"] = remoraID();
    mdata["diveId"] = ID;
    mdata["startTime"] = time;
    mdata["startLat"] = lat;
    mdata["startLng"] = lng;
    mdata["freq"] = freq;

    JsonArray schema = mdata.createNestedArray();
    copyArray(recordSchema, schema);

    String buffer;
    serializeJson(mdata, buffer);
    return storage->writeFile(String("/" + ID + "/metadata.json").c_str(), buffer.c_str());
}

int Dive::writeMetadataEnd(long time, double lat, double lng)
{
    StaticJsonDocument<1024> metadataJson;

    String data;
    data = storage->readFile(String("/" + ID + "/metadata.json").c_str());
    if (data == "")
    {
        Serial.println("Failed to read meatadata to save dive!");
        return -1;
    }

    deserializeJson(metadataJson, data);

    metadataJson["endTime"] = time;
    metadataJson["endLat"] = lat;
    metadataJson["endLng"] = lng;
    metadataJson["numberOfSilos"] = order;

    String buffer;
    serializeJson(metadataJson, buffer);
    return storage->writeFile(String("/" + ID + "/metadata.json").c_str(), buffer.c_str());
}

int Dive::updateIndex()
{
    // this should only happen to a new device
    if (storage->findFile(indexPath) == -1)
    {
        // TODO need a log for this

        DynamicJsonDocument index(indexByteSize);

        // JsonArray dives = index.to<JsonArray>();
        // JsonObject dive = dives.createNestedObject();

        String buffer;
        serializeJson(index, buffer);

        return storage->writeFile(indexPath, buffer);
    }
    else
    {
        String index = storage->readFile(indexPath);
        if (index == "")
        {
            Serial.println("Could not read index file");
            return -1;
        }
        else
        {
            DynamicJsonDocument newIndex(indexByteSize);

            deserializeJson(newIndex, index);
            JsonArray dives = newIndex.to<JsonArray>();
            JsonObject dive = dives.createNestedObject();
            dive["id"] = ID;
            dive["uploadedAt"] = 0;

            String buffer;
            serializeJson(newIndex, buffer);

            return storage->writeFile(indexPath, buffer);
        }
    }
}

String Dive::createID(long time)
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

void Dive::deleteID(String ID)
{
    storage->removeDirectory("/" + ID);
}