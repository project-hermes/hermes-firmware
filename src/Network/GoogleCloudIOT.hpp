#include <Network/Cloud.hpp>

using namespace std;

class GoogleCloudIOT : public Cloud
{
public:
    GoogleCloudIOT();
    int connect();
    int disconnect();
    int upload(String payload);
    int ota();
};