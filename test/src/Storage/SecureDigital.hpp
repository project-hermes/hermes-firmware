#ifndef SECUREDIGITAL_HPP
#define SECUREDIGITAL_HPP

#include <SD.h>
#include <FS.h>

#include <Storage/Storage.hpp>

using namespace std;

class SecureDigital : public Storage
{
public:
    SecureDigital()
    {
        SPI.begin(18, 19, 23);
        delay(10);
        if (!SD.begin(5))
        {
            Serial.println("Card Mount Failed");
            return;
        }
        uint8_t cardType = SD.cardType();

        if (cardType == CARD_NONE)
        {
            Serial.println("No SD card attached");
            return;
        }

        Serial.print("SD Card Type: ");
        if (cardType == CARD_MMC)
        {
            Serial.println("MMC");
        }
        else if (cardType == CARD_SD)
        {
            Serial.println("SDSC");
        }
        else if (cardType == CARD_SDHC)
        {
            Serial.println("SDHC");
        }
        else
        {
            Serial.println("UNKNOWN");
        }
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("SD Card Size: %lluMB\n", cardSize);
    };

    int makeDirectory(String path)
    {
        if (SD.mkdir(path.c_str()))
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }

    int removeDirectory(String path){
        if(SD.rmdir(path.c_str())){
            return 0;
        }else{
            return -1;
        }
    }

    String readFile(String path){
        String data;
        File file = SD.open(path);
        if (!file)
        {
            return "";
        }

        while(file.available()){
            data = data + file.read();
        }

        file.close();

        return data;
    }

    int writeFile(String path, String data){
        File file = SD.open(path, FILE_WRITE);
        if(!file){
            return -1;
        }

        if(file.print(data)){
            file.close();
            return 0;
        }else{
            file.close();
            return -1;
        }
    }

    int appendFile(String path, String data){
        File file = SD.open(path, FILE_APPEND);
        if(!file){
            return -1;
        }

        if(file.print(data)){
            file.close();
            return 0;
        }else{
            file.close();
            return -1;
        }
    }

    int renameFile(String pathA, String pathB){
        if(SD.rename(pathA, pathB)){
            return 0;
        }else{
            return -1;
        }
    }

    int deleteFile(String path){
        if(SD.remove(path)){
            return 0;
        }else{
            return -1;
        }
    }
};

#endif