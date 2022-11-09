#include <Connect.hpp>

int uploadDives(SecureDigital sd)
{
    unsigned long bddID = 0, siloID = 0;
    bool error = false;
    int count = 0;
    StaticJsonDocument<1024> indexJson;
    sd = SecureDigital();

    String data;
    data = sd.readFile(indexPath);
    if (data == "")
    {
        log_e("Could not read index file to upload dives");
        return -2;
    }
    deserializeJson(indexJson, data);
    JsonObject root = indexJson.as<JsonObject>();
    for (JsonObject::iterator it = root.begin(); it != root.end(); ++it)
    {
        error = false;
        String ID = it->key().c_str();

        JsonObject dive = indexJson[ID].as<JsonObject>();
        const int uploaded = dive["uploaded"];
        if (uploaded != 0)
        {
            continue;
        }

        String path = "/" + ID + "/metadata.json";
        String metadata = sd.readFile(path);

        // check if metadata already uploaded
        bddID = checkId(metadata);

        // if not, post metadata and get new id
        if (bddID == 0)
        {
            while (bddID <= 0 && count < POST_RETRY)
            {
                count++;
                bddID = postMetadata(metadata);
                if (bddID > 0)
                {
                    // update metadata with bdd ID on SD card
                    sd.writeFile(path, updateId(metadata, bddID));
                }
            }
        }

        // error during metadata upload
        if (bddID < 0)
        {
            error = true;
        }
        else
        {
            log_i("Metadata posted, start post records");

            int i = 0;
            path = "/" + ID + "/silo0.json";
            String records = "";

            while (sd.findFile(path) == 0)
            {
                path = "/" + ID + "/silo" + i + ".json";
                records = sd.readFile(path);

                if (records != "")
                {
                    // check if silo already uploaded
                    siloID = checkId(records);
                    if (siloID == 0)
                    {
                        // add bdd ID before upload
                        records = updateId(records, bddID);

                        count = 0;
                        int code = 0;
                        while (code != 200 && count < POST_RETRY)
                        {
                            if (postRecordData(records, bddID) != 200) // post silos
                            {
                                error = true;
                                log_e("Silo %d not posted", i);
                            }
                            else
                            {
                                log_i("Silo %d posted", i);
                                // update silo with bdd ID on SD card
                                sd.writeFile(path, records);
                            }

                            count++;
                        }
                    }
                }
                else
                    log_i("Silo %d empty, skipped", i);

                i++;
            }
        }
        if (!error)
        {
            // try PUT, if return 200 then "uploaded"=1
            dive["uploaded"] = 1;
        }
    }

    String buffer;
    serializeJson(indexJson, buffer);

    return sd.writeFile(indexPath, buffer);
}

unsigned long postMetadata(String data)
{
    unsigned long id = 0;
    if ((WiFi.status() == WL_CONNECTED))
    {

        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();

        if (!http.begin(client, metadataURL))
        {
            log_e("BEGIN FAILED...");
        }

        http.setAuthorization(user.c_str(), password.c_str());
        http.addHeader("Content-Type", "application/json");
        int code = http.POST(data.c_str());
        log_i("HTTP RETURN = %d", code);

        if (code != 200)
            return -3;
        else
        {
            String response = http.getString().c_str();

            // convert getString() into id
            StaticJsonDocument<1024> responseJson;
            deserializeJson(responseJson, response);
            id = responseJson["id"];
            log_i("ID RETURN = %d", id);

            return id;
        }

        // Disconnect
        http.end();
    }
    else
    {
        log_e("****** NO WIFI!!");
        return -1;
    }
    return -2;
}

int postRecordData(String data, unsigned long id)
{

    if ((WiFi.status() == WL_CONNECTED))
    {

        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();

        if (!http.begin(client, recordURL))
        {
            log_e("BEGIN FAILED...");
        }

        http.setAuthorization(user.c_str(), password.c_str());
        http.addHeader("Content-Type", "application/json");
        int code = http.POST(data.c_str());
        log_i("HTTP RETURN = %d", code);
        log_i("HTTP RETURN = %s", http.getString().c_str());

        // Disconnect
        http.end();

        return code;
    }
    else
    {
        log_e("****** NO WIFI!!");
        return -1;
    }
    return -2;
}

void startPortal(SecureDigital sd)
{
    WebServer Server;
    AutoConnect Portal(Server);

    log_v("starting config portal...");

    AutoConnectConfig acConfig("Remora Config", "cousteau");
    // acConfig.hostName = String("remora");
    // acConfig.homeUri = "https://www.google.fr";
    acConfig.autoReconnect = true;
    acConfig.autoReset = false;
    acConfig.portalTimeout = 15 * 60 * 1000;
    acConfig.title = "Remora Config";
    acConfig.ticker = true;
    acConfig.tickerPort = GPIO_LED3;
    acConfig.tickerOn = LOW;
    Portal.config(acConfig);
    Portal.begin();
    bool error = false;

    while (WiFi.status() == WL_DISCONNECTED)
    {
        Portal.handleClient();
    }
    log_i("Adresse IP : %s", WiFi.localIP().toString().c_str());

    // detach interrupt to keep remora alive during upload and ota process even if usb is disconnected
    detachInterrupt(GPIO_VCC_SENSE);

    pinMode(GPIO_LED1, OUTPUT);
    digitalWrite(GPIO_LED2, HIGH);

    log_v("Wifi connected, start upload dives");

    if (uploadDives(sd) != SUCCESS)
        error = true;

    log_v("Upload finished, start OTA");
    if (ota(sd) != SUCCESS)
        error = true;

    pinMode(GPIO_LED1, OUTPUT);
    digitalWrite(GPIO_LED1, error);
    digitalWrite(GPIO_LED2, LOW);
    log_v("OTA finished, waiting for usb disconnection");

    while (WiFi.status() == WL_CONNECTED && digitalRead(GPIO_VCC_SENSE))
    {
        Portal.handleClient();
    }

    log_v("USB disconnected, go back to sleep");
    sleep(DEFAULT_SLEEP);
}

int ota(SecureDigital sd)
{
    sd = SecureDigital();

    String firmwareName;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    String updateURL;
    log_i("Adresse IP : %s", WiFi.localIP().toString().c_str());

    http.setAuthorization(user.c_str(), password.c_str());

    if (http.begin(firmwareURL))
    {
        if (http.GET() == 200)
        {
            firmwareName = http.getString();

            StaticJsonDocument<1024> firmwareJson;
            deserializeJson(firmwareJson, firmwareName);
            const char *name = firmwareJson["name"];
            String version = name;
            const char *url = firmwareJson["url"];
            updateURL = url;
            log_d("Name = %s", version.c_str());

            version.remove(0, 10);

            log_i("Firmware = %1.2f", version.toFloat());

            if (version.toFloat() <= FIRMWARE_VERSION + 0.00001)
            {
                http.end();
                log_i("Will not update as I am version:%1.2f and you are offering version:%1.2f\n", version.toFloat(), FIRMWARE_VERSION);
                return SUCCESS;
            }
            else
            {
                // Start update
                size_t written = 0;
                size_t gotten = 1;
                if (http.begin(client, updateURL))
                {
                    if (http.GET() == 200)
                    {
                        gotten = http.getSize();
                        if (!Update.begin(gotten))
                        {
                            log_e("Firmware file too big at %d\n", gotten);
                            http.end();
                            return FIRMWARE_SIZE_ERROR;
                        }
                        log_v("atempting to update...");
                        written = Update.writeStream(http.getStream());
                    }
                    http.end();
                }
                else
                {
                    log_e("could not get update file");
                    http.end();
                    return GET_FIRMWARE_ERROR;
                }

                if (written == gotten)
                {
                    log_i("Written : %d successfully", written);
                }
                else
                {
                    log_e("Written only : %d/%d . Retry?", written, gotten);
                }

                if (Update.end())
                {
                    log_v("OTA done!");
                    if (Update.isFinished())
                    {
                        log_v("Update successfully completed. Rebooting.");
                        // Write version on SD card
                        String path = "/version.txt";

                        sd.writeFile(path, version);
                    }
                    else
                    {
                        log_e("Update not finished? Something went wrong!");
                    }
                }
                else
                {
                    log_e("Error Occurred. Error #: %d", Update.getError());
                }
            }
        }
        else
        {
            log_e("could not contact cloud function");
            http.end();
            return CONNECTION_ERROR;
        }
    }
    else
    {
        log_e("could not begin http client");
        return HTTP_BEGIN_ERROR;
    }
    http.end();
    return SUCCESS;
}

String updateId(String data, unsigned long bddID)
{
    DynamicJsonDocument dataJson(jsonSize);
    deserializeJson(dataJson, data);
    dataJson["id"] = bddID;
    String returnJson = "";
    serializeJson(dataJson, returnJson);
    return returnJson;
}

unsigned long checkId(String data)
{
    unsigned long id = 0;
    DynamicJsonDocument dataJson(jsonSize);
    deserializeJson(dataJson, data);
    id = dataJson["id"];
    return id;
}