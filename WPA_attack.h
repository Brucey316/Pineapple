#ifndef WPA_ATTACK_H
#define WPA_ATTACK_H

#include "wifi.h"

void runDeauth(char* ap_bssid, char* client_bssid);
void checkKey(AP* victim_APs, int num_victims, char* filename);

#endif
