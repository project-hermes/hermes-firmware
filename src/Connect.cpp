#include <Connect.hpp>

void startPortal(SecureDigital sd)
{
    WebServer Server;
    AutoConnect Portal(Server);

    Serial.printf("starting config portal...\n");
    AutoConnectConfig acConfig("Remora Config", "cousteau");
    acConfig.hostName = "remora";
    acConfig.homeUri = "/remora";
    acConfig.autoReconnect = true;
    acConfig.autoReset = false;
    acConfig.portalTimeout = 15 * 60 * 1000;
    acConfig.title = "Remora Config";
    acConfig.ticker = true;
    acConfig.tickerPort = GPIO_LED3;
    acConfig.tickerOn = LOW;
    Portal.config(acConfig);
    Portal.begin();

    while (WiFi.status() == WL_DISCONNECTED)
    {
        Portal.handleClient();
    }
    Serial.println(WiFi.localIP());

    Serial.println("Wifi connected, start upload dives");
    uploadDives(sd);
    // Serial.println("disconnecting");
    // cloud.disconnect();
    return;

    // turned off for testing
    /*if (digitalRead(GPIO_VCC_SENSE) == 1)
    {
        while (WiFi.status() == WL_DISCONNECTED)
        {
            Portal.handleClient();
            if (digitalRead(GPIO_VCC_SENSE) == 0)
            {
                return;
            }
        }
        ota();
        sendJson();
    }
    while (digitalRead(GPIO_VCC_SENSE) == 1)
    {
        Portal.handleClient();
    }*/
}

int uploadDives(SecureDigital sd)
{

    GoogleCloudIOT cloud = GoogleCloudIOT();
    cloud.connect();

    StaticJsonDocument<1024> indexJson;
    sd = SecureDigital();

    String data;
    data = sd.readFile("/index.json");
    if (data == "")
    {
        Serial.println("Could not read index file to upload dives");
        return -1;
    }
    deserializeJson(indexJson, data);
    log_v("\nNb dives recorded = %d\n");
    for (int i = 0; i < indexJson.size(); i++)
    {
        JsonObject dive = indexJson[i].as<JsonObject>();
        if (dive["uploadedAt"] != 0)
        {
            continue;
        }
        StaticJsonDocument<512> diveMetadataJson;
        String diveId = dive["id"];
        String diveMetadata = sd.readFile(String("/" + diveId + "/metadata.json"));
        if (diveMetadata == "")
        {
            Serial.println("could not load dive metadata for dive " + diveId);
            return -1;
        }

        deserializeJson(diveMetadataJson, diveMetadata);

        int numberOfSilos = diveMetadataJson["numberOfSilos"];

        for (int i = 1; i < numberOfSilos + 1; i++)
        {
            String diveSilo = sd.readFile(String("/" + diveId + "/" + "silo_" + i + ".json"));
            if (diveSilo == "")
            {
                Serial.println("could not load dive silo " + String(i) + " for dive " + diveId);
                return -1;
            }
            else
            {
                Serial.println("Loaded silo " + String(i) + " of dive " + diveId);
            }

            if (cloud.upload("Test dive data") == -1)
            {
                Serial.println("Could not upload silo " + String(i) + " of dive " + diveId);
                return -1;
            }
            else
            {
                Serial.println("Uploaded silo " + String(i) + " of dive " + diveId);
            }
        }
        // TODO update the index so the dive does not reuplaod
    }
    return 0;
}

void ota()
{
    String cloudFunction = "http://us-central1-project-hermes-staging.cloudfunctions.net/ota";
    String bucketURL = "http://storage.googleapis.com/remora-firmware/";
    String version;
    HTTPClient http;

    Serial.println(WiFi.localIP());

    if (http.begin(cloudFunction))
    {
        if (http.GET() == 200)
        {
            version = http.getString();
            if (version.toInt() <= FIRMWARE_VERSION)
            {
                http.end();
                Serial.printf("Will not update as I am version:%ld and you are offering version:%d\n", version.toInt(), FIRMWARE_VERSION);
                return;
            }
        }
        else
        {
            Serial.println("could not contact cloud function");
            http.end();
            return;
        }
    }
    else
    {
        Serial.println("could not begin http client");
    }
    http.end();

    size_t written = 0;
    size_t gotten = 1;
    String firmware_url = bucketURL + "firmware_" + FIRMWARE_VERSION + ".bin";
    if (http.begin(firmware_url))
    {
        if (http.GET() == 200)
        {
            gotten = http.getSize();
            if (!Update.begin(gotten))
            {
                Serial.printf("Firmware file too big at %d\n", gotten);
                http.end();
                return;
            }
            Serial.println("atempting to update...");
            written = Update.writeStream(http.getStream());
        }
        http.end();
    }
    else
    {
        Serial.println("could not get update file");
        http.end();
        return;
    }

    if (written == gotten)
    {
        Serial.println("Written : " + String(written) + " successfully");
    }
    else
    {
        Serial.println("Written only : " + String(written) + "/" + String(gotten) + ". Retry?");
    }

    if (Update.end())
    {
        Serial.println("OTA done!");
        if (Update.isFinished())
        {
            Serial.println("Update successfully completed. Rebooting.");
            ESP.restart();
        }
        else
        {
            Serial.println("Update not finished? Something went wrong!");
        }
    }
    else
    {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }
}
