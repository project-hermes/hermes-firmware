#ifndef STORAGE_H
#define STORAGE_H

#include <FS.h>

using namespace std;

class Storage
{
public:
    virtual int makeDirectory(String path) = 0;
    virtual int removeDirectory(String path) = 0;

    virtual String readFile(String path) = 0;
    virtual int writeFile(String path, String data) = 0;
    virtual int appendFile(String path, String data) = 0;
    virtual int renameFile(String pathA, String pathB) = 0;
    virtual int deleteFile(String path) = 0;
};

#endif