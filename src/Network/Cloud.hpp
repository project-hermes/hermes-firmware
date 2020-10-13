#ifndef CLOUD_H
#define CLOUD_H

using namespace std;

class Cloud
{
public:
    virtual int connect();
    virtual int disconnect();
    virtual int upload();
    virtual int ota();
};

#endif