#ifndef WPA_ATTACK_H
#define WPA_ATTACK_H

#include "wifi.h"

#define STEP_SCANNING           1
#define STEP_ATTACKING          2
#define STEP_HANDSHAKE_CHECKING 3
#define STEP_SAVING_PCAP        4

void runDeauth(char* ap_bssid, char* client_bssid);
bool checkKey(AP* victim_APs, int num_victims, char* filename);

#endif
