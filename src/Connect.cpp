#include <Connect.hpp>
#include <ArduinoJson.h>
#include <Dive.hpp>

int uploadDives(SecureDigital sd)
{
    bool error = false;
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

        String metadata = sd.readFile("/" + ID + "/metadata.json");
        log_v("RECORDS = %s ", metadata.c_str());
        if (post(metadata, true) != 200) // post metadata
            error = true;
        else
            log_i("Metadata posted");

        int i = 0;
        String path = "/" + ID + "/silo0.json";

        String records = "";
        while (sd.findFile(path) == 0)
        {
            String records = sd.readFile(path);
            log_d("SILO = %s", records.c_str());
            if (records != "")
            {
                if (post(records) != 200) // post silos
                {
                    error = true;
                    log_e("Silo %d not posted", i);
                }
                else
                    log_i("Silo %d posted", i);
            }
            else
            {
                log_i("Silo %d empty, skipped", i);
            }
            i++;
            path = "/" + ID + "/silo" + i + ".json";
        }

        if (!error)
            dive["uploaded"] = 1;
    }

    String buffer;
    serializeJson(indexJson, buffer);

    return sd.writeFile(indexPath, buffer);
}

int post(String data, bool metadata)
{

    if ((WiFi.status() == WL_CONNECTED))
    {

        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();

        if (!http.begin(client, metadata ? metadataURL : recordURL))
        {
            log_e("BEGIN FAILED...");
        }
        // Specify Authorization-type header
        // String recv_token = "eyJ0eXAiOiJK..."; // Complete Bearer token
        // recv_token = "Bearer " + recv_token;	// Adding "Bearer " before token

        // http.addHeader("Authorization", recv_token); // Adding Bearer token as HTTP header
        http.addHeader("Content-Type", "application/json");
        int code = http.POST(data.c_str());
        log_i("HTTP RETURN = %d", code);

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
    if (ota() != SUCCESS)
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
    sleep(false);
}

int ota()
{
    String firmwareName;
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    String updateURL;
    log_i("Adresse IP : %s", WiFi.localIP().toString().c_str());

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