#include <Dive.hpp>
#include <Storage/Storage.hpp>

Dive::Dive(void) {}

Dive::Dive(Storage *s)
{
    storage = s;
}

String Dive::Start(long time, lat lat, lng lng)
{
    Serial.println(time);
    ID = createID(time);
    diveRecords = new Record[siloRecordSize];
    if(writeMetadataStart(time,lat,lng, 1)==-1){
        return "";
    }
    return ID;
}

String Dive::End(long time, lat lat, lng lng){
    Serial.println(time);
    if(writeMetadataEnd(time, lat, lng)==-1){
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
        if(writeSilo()==-1){
            Serial.println("error saving silo");
            return -1;
        }
        delete[] diveRecords;
        diveRecords = new Record[siloRecordSize];
        currentRecords = 0;
    }
    return 0;
}