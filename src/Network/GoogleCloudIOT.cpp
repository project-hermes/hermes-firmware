#include <Network/GoogleCloudIOT.hpp>
#include <Network/GoogleCloudGlobal.hpp>

GoogleCloudIOT::GoogleCloudIOT() {}

int GoogleCloudIOT::connect()
{
    setupCloudIoT();
    mqttConnect();
    mqtt->loop();
    return 0;
}

int GoogleCloudIOT::disconnect()
{
    mqtt->loop();
    mqttDisconnect();
    return 0;
}

int GoogleCloudIOT::upload(String payload)
{
    int code;
    if (mqtt->publishTelemetry(payload))
    {
        code = 0;
    }
    else
    {
        code = -1;
    }
    
    if(!mqttClient->connected()){
        mqtt->mqttConnect();
    }
    else
    {
        mqtt->loop();
    }

    return code;
}

int GoogleCloudIOT::ota()
{
    return 0;
}