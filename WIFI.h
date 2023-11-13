#ifndef WIFI_H
#define WIFI_H

#include <inttypes.h>

struct BSSID{
    uint8_t oui[3];
    uint8_t nic[3];
} BSSID;

typedef struct AP{
    //BSSID of an AP
    struct BSSID bssid;
    //ESSID of AP
    char* essid;
    //list of clients
    struct Client* clients;
    //number of clients (up to 255 per AP)
    uint8_t num_clients;
    //has a key been captured for the AP
    bool keyCaptured;
    //has it been attacked before?
    bool attacked;
}AP;
typedef struct Client{
    struct BSSID bssid;
    int attack_count;
}Client;

//converts string BSSID to Byte array BSSID
#define PACK_BSSID(old_bssid, new_bssid) do{ \
    sscanf(old_bssid, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", \
        &(new_bssid)->oui[0],&(new_bssid)->oui[1],&(new_bssid)->oui[2],\
        &(new_bssid)->nic[0],&(new_bssid)->nic[1],&(new_bssid)->nic[2]);\
} while (0)
//converts and prints byte BSSID to string BSSID
#define BSSID_TO_STRING(bssid, sbssid) do{ \
    sprintf(sbssid, "%02X:%02X:%02X:%02X:%02X:%02X", \
        (bssid)->oui[0],(bssid)->oui[1],(bssid)->oui[2],\
        (bssid)->nic[0],(bssid)->nic[1],(bssid)->nic[2]); \
} while (0)

#endif