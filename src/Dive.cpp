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
    Serial.print(currentRecords),Serial.print(":\tTemp = "), Serial.print(r.Temp), Serial.print("\tDepth = "), Serial.println(r.Depth);

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

void Dive::postSecure()
{

    const static char *test_root_ca PROGMEM =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIIujCCBqKgAwIBAgITMwAxS6DhmVCLBf6MWwAAADFLoDANBgkqhkiG9w0BAQwF\n"
        "ADBZMQswCQYDVQQGEwJVUzEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9u\n"
        "MSowKAYDVQQDEyFNaWNyb3NvZnQgQXp1cmUgVExTIElzc3VpbmcgQ0EgMDEwHhcN\n"
        "MjIwMzE0MTgzOTU1WhcNMjMwMzA5MTgzOTU1WjBqMQswCQYDVQQGEwJVUzELMAkG\n"
        "A1UECBMCV0ExEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBD\n"
        "b3Jwb3JhdGlvbjEcMBoGA1UEAwwTKi5henVyZXdlYnNpdGVzLm5ldDCCASIwDQYJ\n"
        "KoZIhvcNAQEBBQADggEPADCCAQoCggEBAM3heDMqn7v8cmh4A9vECuEfuiUnKBIw\n"
        "7y0Sf499Z7WW92HDkIvV3eJ6jcyq41f2UJcG8ivCu30eMnYyyI+aRHIedkvOBA2i\n"
        "PqG78e99qGTuKCj9lrJGVfeTBJ1VIlPvfuHFv/3JaKIBpRtuqxCdlgsGAJQmvHEn\n"
        "vIHUV2jgj4iWNBDoC83ShtWg6qV2ol7yiaClB20Af5byo36jVdMN6vS+/othn3jG\n"
        "pn+NP00DWYbP5y4qhs5XLH9wQZaTUPKIaUxmHewErcM0rMAaWl8wMqQTeNYf3l5D\n"
        "ax50yuEg9VVjtbDdSmvOkslGpVqsOl1NrmyN7gCvcvcRUQcxIiXJQc0CAwEAAaOC\n"
        "BGgwggRkMIIBfwYKKwYBBAHWeQIEAgSCAW8EggFrAWkAdgCt9776fP8QyIudPZwe\n"
        "PhhqtGcpXc+xDCTKhYY069yCigAAAX+Jw/reAAAEAwBHMEUCIE8AAjvwO4AffPn7\n"
        "un67WykJ2hGB4n8qJE7pk4QYjWW+AiEA/pio1E9ALt30Kh/Ga4gRefH1ILbQ8n4h\n"
        "bHFatezIcvYAdwB6MoxU2LcttiDqOOBSHumEFnAyE4VNO9IrwTpXo1LrUgAAAX+J\n"
        "w/qlAAAEAwBIMEYCIQCdbj6FOX6wK+dLoqjWKuCgkKSsZsJKpVik6HjlRgomzQIh\n"
        "AM7mYp5dBFmNLas3fFcP0rMMK+17n8u0GhFH2KpkPr1SAHYA6D7Q2j71BjUy51co\n"
        "vIlryQPTy9ERa+zraeF3fW0GvW4AAAF/icP6jgAABAMARzBFAiAhjTz3PBjqRrpY\n"
        "eH7us44lESC7c0dzdTcehTeAwmEyrgIhAOCaqmqA+ercv+39jzFWkctG36bazRFX\n"
        "4gGNiKU0bctcMCcGCSsGAQQBgjcVCgQaMBgwCgYIKwYBBQUHAwIwCgYIKwYBBQUH\n"
        "AwEwPAYJKwYBBAGCNxUHBC8wLQYlKwYBBAGCNxUIh73XG4Hn60aCgZ0ujtAMh/Da\n"
        "HV2ChOVpgvOnPgIBZAIBJTCBrgYIKwYBBQUHAQEEgaEwgZ4wbQYIKwYBBQUHMAKG\n"
        "YWh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0\n"
        "JTIwQXp1cmUlMjBUTFMlMjBJc3N1aW5nJTIwQ0ElMjAwMSUyMC0lMjB4c2lnbi5j\n"
        "cnQwLQYIKwYBBQUHMAGGIWh0dHA6Ly9vbmVvY3NwLm1pY3Jvc29mdC5jb20vb2Nz\n"
        "cDAdBgNVHQ4EFgQUiiks5RXI6IIQccflfDtgAHndN7owDgYDVR0PAQH/BAQDAgSw\n"
        "MHwGA1UdEQR1MHOCEyouYXp1cmV3ZWJzaXRlcy5uZXSCFyouc2NtLmF6dXJld2Vi\n"
        "c2l0ZXMubmV0ghIqLmF6dXJlLW1vYmlsZS5uZXSCFiouc2NtLmF6dXJlLW1vYmls\n"
        "ZS5uZXSCFyouc3NvLmF6dXJld2Vic2l0ZXMubmV0MAwGA1UdEwEB/wQCMAAwZAYD\n"
        "VR0fBF0wWzBZoFegVYZTaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9j\n"
        "cmwvTWljcm9zb2Z0JTIwQXp1cmUlMjBUTFMlMjBJc3N1aW5nJTIwQ0ElMjAwMS5j\n"
        "cmwwZgYDVR0gBF8wXTBRBgwrBgEEAYI3TIN9AQEwQTA/BggrBgEFBQcCARYzaHR0\n"
        "cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9Eb2NzL1JlcG9zaXRvcnkuaHRt\n"
        "MAgGBmeBDAECAjAfBgNVHSMEGDAWgBQPIF3XoVeV25LPK9DHwncEznKAdjAdBgNV\n"
        "HSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwDQYJKoZIhvcNAQEMBQADggIBAKtk\n"
        "4nEDfqxbP80uaoBoPaeeX4G/tBNcfpR2sf6soW8atAqGOohdLPcE0n5/KJn+H4u7\n"
        "CsZdTJyUVxBxAlpqAc9JABl4urWNbhv4pueGBZXOn5K5Lpup/gp1HhCx4XKFno/7\n"
        "T22NVDol4LRLUTeTkrpNyYLU5QYBQpqlFMAcvem/2seiPPYghFtLr5VWVEikUvnf\n"
        "wSlECNk84PT7mOdbrX7T3CbG9WEZVmSYxMCS4pwcW3caXoSzUzZ0H1sJndCJW8La\n"
        "9tekRKkMVkN558S+FFwaY1yARNqCFeK+yiwvkkkojqHbgwFJgCFWYy37kFR9uPiv\n"
        "3sTHvs8IZ5K8TY7rHk3pSMYqoBTODCs7wKGiByWSDMcfAgGBzjt95SKfq0p6sj0C\n"
        "+HWFiyKR+PTi2esFP9Vr9sC9jfRM6zwa7KnONqLefHauJPdNMt5l1FQGWvyco4IN\n"
        "lwK3Z9FfEOFZA4YcjsqnkNacKZqLjgis3FvD8VPXETgRuffVc75lJxH6WmkwqdXj\n"
        "BlU8wOcJyXTmM1ehYpziCpWvGBSEIsFuK6BC/iBnQEuWKdctAdbHIDlLctGgDWjx\n"
        "xYDPZ/TtORGL8YaDnj6QHeOURIAHCtt6NCWKV6OR2HtMx+tCEvfi5ION1dyJ9hAX\n"
        "+4K9FXc71ab7tdV/GLPkWc8Q0x1nk7ogDYcqKbiF\n"
        "-----END CERTIFICATE-----\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIF8zCCBNugAwIBAgIQCq+mxcpjxFFB6jvh98dTFzANBgkqhkiG9w0BAQwFADBh\n"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
        "MjAeFw0yMDA3MjkxMjMwMDBaFw0yNDA2MjcyMzU5NTlaMFkxCzAJBgNVBAYTAlVT\n"
        "MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKjAoBgNVBAMTIU1pY3Jv\n"
        "c29mdCBBenVyZSBUTFMgSXNzdWluZyBDQSAwMTCCAiIwDQYJKoZIhvcNAQEBBQAD\n"
        "ggIPADCCAgoCggIBAMedcDrkXufP7pxVm1FHLDNA9IjwHaMoaY8arqqZ4Gff4xyr\n"
        "RygnavXL7g12MPAx8Q6Dd9hfBzrfWxkF0Br2wIvlvkzW01naNVSkHp+OS3hL3W6n\n"
        "l/jYvZnVeJXjtsKYcXIf/6WtspcF5awlQ9LZJcjwaH7KoZuK+THpXCMtzD8XNVdm\n"
        "GW/JI0C/7U/E7evXn9XDio8SYkGSM63aLO5BtLCv092+1d4GGBSQYolRq+7Pd1kR\n"
        "EkWBPm0ywZ2Vb8GIS5DLrjelEkBnKCyy3B0yQud9dpVsiUeE7F5sY8Me96WVxQcb\n"
        "OyYdEY/j/9UpDlOG+vA+YgOvBhkKEjiqygVpP8EZoMMijephzg43b5Qi9r5UrvYo\n"
        "o19oR/8pf4HJNDPF0/FJwFVMW8PmCBLGstin3NE1+NeWTkGt0TzpHjgKyfaDP2tO\n"
        "4bCk1G7pP2kDFT7SYfc8xbgCkFQ2UCEXsaH/f5YmpLn4YPiNFCeeIida7xnfTvc4\n"
        "7IxyVccHHq1FzGygOqemrxEETKh8hvDR6eBdrBwmCHVgZrnAqnn93JtGyPLi6+cj\n"
        "WGVGtMZHwzVvX1HvSFG771sskcEjJxiQNQDQRWHEh3NxvNb7kFlAXnVdRkkvhjpR\n"
        "GchFhTAzqmwltdWhWDEyCMKC2x/mSZvZtlZGY+g37Y72qHzidwtyW7rBetZJAgMB\n"
        "AAGjggGtMIIBqTAdBgNVHQ4EFgQUDyBd16FXlduSzyvQx8J3BM5ygHYwHwYDVR0j\n"
        "BBgwFoAUTiJUIBiV5uNu5g/6+rkS7QYXjzkwDgYDVR0PAQH/BAQDAgGGMB0GA1Ud\n"
        "JQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/AgEAMHYG\n"
        "CCsGAQUFBwEBBGowaDAkBggrBgEFBQcwAYYYaHR0cDovL29jc3AuZGlnaWNlcnQu\n"
        "Y29tMEAGCCsGAQUFBzAChjRodHRwOi8vY2FjZXJ0cy5kaWdpY2VydC5jb20vRGln\n"
        "aUNlcnRHbG9iYWxSb290RzIuY3J0MHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6Ly9j\n"
        "cmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5jcmwwN6A1oDOG\n"
        "MWh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5j\n"
        "cmwwHQYDVR0gBBYwFDAIBgZngQwBAgEwCAYGZ4EMAQICMBAGCSsGAQQBgjcVAQQD\n"
        "AgEAMA0GCSqGSIb3DQEBDAUAA4IBAQAlFvNh7QgXVLAZSsNR2XRmIn9iS8OHFCBA\n"
        "WxKJoi8YYQafpMTkMqeuzoL3HWb1pYEipsDkhiMnrpfeYZEA7Lz7yqEEtfgHcEBs\n"
        "K9KcStQGGZRfmWU07hPXHnFz+5gTXqzCE2PBMlRgVUYJiA25mJPXfB00gDvGhtYa\n"
        "+mENwM9Bq1B9YYLyLjRtUz8cyGsdyTIG/bBM/Q9jcV8JGqMU/UjAdh1pFyTnnHEl\n"
        "Y59Npi7F87ZqYYJEHJM2LGD+le8VsHjgeWX2CJQko7klXvcizuZvUEDTjHaQcs2J\n"
        "+kPgfyMIOY1DMJ21NxOJ2xPRC/wAh/hzSBRVtoAnyuxtkZ4VjIOh\n"
        "-----END CERTIFICATE-----\n";

    const char *ssid = "Freebox-399EFC";
    const char *password = "dtndkqtx2wqk4cd2qrndkf";
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());

    String json;
    String records = storage->readFile("/test.json");
    Serial.print("String = "), Serial.println(records);
    Serial.print("C_str = "), Serial.println(records.c_str());

    if ((WiFi.status() == WL_CONNECTED))
    {

        HTTPClient http;
        WiFiClientSecure client;
        client.setCACert(test_root_ca);
        // client.setInsecure();

        if (!http.begin(client, "https://project-hermes.azurewebsites.net/api/Feed"))
        {
            Serial.println("BEGIN FAILED...");
        }

        http.addHeader("Content-Type", "application/json");
        int code = http.POST(records.c_str());
        // int code = http.POST("{\"id\":0,\"data\":\"string\",\"creationDate\":\"2022-04-13T21:22:43.853Z\"}");

        if (code < 0)
        {
            Serial.print("ERROR: ");
            Serial.println(code);
        }
        else
        {
            // Read response
            http.writeToStream(&Serial);
            Serial.flush();
        }

        // Disconnect
        http.end();
    }
    else
    {
        Serial.println("****** NO WIFI!!");
    }
}