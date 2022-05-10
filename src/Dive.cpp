#include <Dive.hpp>
#include <Storage/Storage.hpp>

RTC_DATA_ATTR char savedID[100];

Dive::Dive(void) {}

Dive::Dive(Storage *s)
{
    storage = s;
}

void Dive::init()
{

    ID = "";
    currentRecords = 0;
    Record *diveRecords;
    metadata.ID = "";
    metadata.startTime = 0;
    metadata.endTime = 0;
    metadata.freq = 0;
    metadata.siloSize = 0;
    metadata.startLat = 0;
    metadata.startLng = 0;
    metadata.endLat = 0;
    metadata.endLng = 0;
    diveRecords->Temp = 0;
    diveRecords->Depth = 0;
}

String Dive::Start(long time, lat lat, lng lng, int freq, bool mode)
{
    init();
    Serial.println(time);
    ID = createID(time);
    saveId(ID);

    diveRecords = new Record[siloRecordSize];
    if (writeMetadataStart(time, lat, lng, freq, mode) == -1)
    {
        return "";
    }
    createIndex();
    return ID;
}

String Dive::End(long time, lat lat, lng lng)
{
    Serial.print("Time = "), Serial.println(time);
    if (writeMetadataEnd(time, lat, lng) == -1)
    {
        return "";
    }
    return ID;
}

int Dive::NewRecord(Record r)
{
    Serial.print(currentRecords), Serial.print(":\tTemp = "), Serial.print(r.Temp), Serial.print("\tDepth = "), Serial.println(r.Depth);

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

int Dive::NewRecordStatic(Record r)
{
    diveRecords[0] = r;

    if (writeStaticRecord() == -1)
    {
        Serial.println("error saving silo");
        return -1;
    }

    return 0;
}

int Dive::writeSilo()
{

    String path = "/" + ID + "/diveRecords.json";

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
        String records = storage->readFile(path);
        if (records == "")
        {
            Serial.println("Could not read previous ID records file");
            return -1;
        }
        else
        {

            DynamicJsonDocument newRecords(siloByteSize);
            deserializeJson(newRecords, records);

            for (int i = 0; i < siloRecordSize; i++)
            {
                JsonArray record = newRecords["records"].createNestedArray();
                record.add(diveRecords[i].Temp);
                record.add(diveRecords[i].Depth);
            }

            String buffer;
            serializeJson(newRecords, buffer);

            return storage->writeFile(path, buffer);
        }
    }
}

int Dive::writeStaticRecord()
{
    ID = getID();

    String path = "/" + ID + "/diveRecords.json";

    // this should only happen to a new dive record
    if (storage->findFile(path) == -1)
    {

        DynamicJsonDocument jsonSilo(siloByteSize);

        jsonSilo["id"] = ID;
        JsonArray records = jsonSilo.createNestedArray("records");

        JsonArray record = records.createNestedArray();
        record.add(diveRecords[0].Temp);
        record.add(diveRecords[0].Depth);

        String buffer;
        serializeJson(jsonSilo, buffer);

        return storage->writeFile(path, buffer);
    }
    else
    {
        String records = storage->readFile(path);
        if (records == "")
        {
            Serial.println("Could not read previous ID records file");
            return -1;
        }
        else
        {

            DynamicJsonDocument newRecords(siloByteSize);
            deserializeJson(newRecords, records);

            JsonArray record = newRecords["records"].createNestedArray();
            record.add(diveRecords[0].Temp);
            record.add(diveRecords[0].Depth);

            String buffer;
            serializeJson(newRecords, buffer);

            return storage->writeFile(path, buffer);
        }
    }
}

int Dive::writeMetadataStart(long time, double lat, double lng, int freq, bool mode)
{
    StaticJsonDocument<1024> mdata;
    storage->makeDirectory("/" + ID);
    String path = "/" + ID + "/diveRecords.json";

    mdata["deviceId"] = remoraID();
    mdata["diveId"] = ID;
    mdata["mode"] = (mode == 1 ? "Static" : "Dynamic");
    mdata["startTime"] = time;
    mdata["startLat"] = lat;
    mdata["startLng"] = lng;
    mdata["freq"] = freq;
    mdata.createNestedArray("records");

    String buffer;
    serializeJson(mdata, buffer);
    return storage->writeFile(path, buffer);
}

int Dive::writeMetadataEnd(long time, double lat, double lng)
{
    StaticJsonDocument<1024> mdata;
    String path = "/" + ID + "/diveRecords.json";

    String data;
    data = storage->readFile(path);
    if (data == "")
    {
        Serial.println("Failed to read meatadata to save dive!");
        return -1;
    }

    deserializeJson(mdata, data);

    mdata["endTime"] = time;
    mdata["endLat"] = lat;
    mdata["endLng"] = lng;

    String buffer;
    serializeJson(mdata, buffer);
    return storage->writeFile(path, buffer);
}

int Dive::createIndex()
{
    // this should only happen to a new device
    if (storage->findFile(indexPath) == -1)
    {
        DynamicJsonDocument index(indexByteSize);

        JsonObject dive = index.createNestedObject(ID);
        dive["uploaded"] = 0;

        String buffer;
        serializeJson(index, buffer);
        Serial.print("BUFFER NEW INDEX = "), Serial.println(buffer);

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
            JsonObject dive = newIndex.createNestedObject(ID);
            dive["uploaded"] = 0;

            String buffer;
            serializeJson(newIndex, buffer);
            Serial.print("BUFFER INDEX = "), Serial.println(buffer);

            return storage->writeFile(indexPath, buffer);
        }
    }
}

void Dive::saveId(String ID)
{
    ID.toCharArray(savedID, ID.length() + 1);
}

String Dive::getID()
{
    Serial.print("Saved ID"), Serial.println(savedID);
    return String(savedID);
}

String Dive::createID(long time)
{
    byte shaResult[32];
    WiFi.mode(WIFI_MODE_STA);
    String unhashed_id = String(time) + WiFi.macAddress() + String(esp_random());
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
    int del = storage->removeDirectory("/" + ID);
    Serial.print("Delete = "), Serial.println(del);
}