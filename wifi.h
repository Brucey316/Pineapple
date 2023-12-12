#ifndef WIFI_H
#define WIFI_H

#include "common_dependancies.h"
#define BSSID_LENGTH 18
#define BSSID_REGEX "[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}"
#define MAX_ATTACK 5
#define DEFAULT_ESSID "<HIDDEN-ESSID>"

typedef union BSSID{
    struct{
        uint8_t oui[3];
        uint8_t nic[3];
    };
    uint8_t mac[6];
} BSSID;

typedef struct AP{
    //BSSID of an AP
    BSSID bssid;
    //ESSID of AP
    char* essid;
    //list of clients
    struct Client* clients;
    //number of clients (up to 255 per AP)
    uint8_t num_clients;
    //has a key been captured for the AP
    bool keyCaptured;
    //index of the channel of the AP
    int channel;
    //has it been attacked before?
    bool attacked;
}AP;
typedef struct Client{
    //bssid
    BSSID bssid; 
    //total number of times attacked
    int attack_count; 
    //attacked on iteration
    bool attacked; 
}Client;

AP createAP(char* name, BSSID bssid, int channel);
void addClient(AP* ap, BSSID bssid);
void destroyAP(AP ap);
char* getWirelessDevice();
char** getCompatibleChannels();
void savePackets(char* capture_file);
void printReport();

//converts string BSSID to Byte array BSSID
#define PACK_BSSID(old_bssid, new_bssid) do{ \
    sscanf(old_bssid, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", \
        &(new_bssid)->oui[0],&(new_bssid)->oui[1],&(new_bssid)->oui[2],\
        &(new_bssid)->nic[0],&(new_bssid)->nic[1],&(new_bssid)->nic[2]);\
} while (0)
//converts byte BSSID to string BSSID
#define BSSID_TO_STRING(bssid, sbssid) do{ \
    sprintf(sbssid, "%02X:%02X:%02X:%02X:%02X:%02X", \
        (bssid)->oui[0],(bssid)->oui[1],(bssid)->oui[2],\
        (bssid)->nic[0],(bssid)->nic[1],(bssid)->nic[2]); \
} while (0)

#endif