#ifndef WAKE_HPP
#define WAKE_HPP

#include <Network/GoogleCloudIOT.hpp>

void wake();

void sleep();

//refactor these out latter
void startPortal();

void ota();

int uploadDives();

void getJWT();

void sendJson();


#endif