#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>

struct BSSID{
    uint8_t oui[3];
    uint8_t nic[3];
} BSSID;

typedef struct AP{
    struct BSSID bssid;
    //name of AP
    char* essid;
    //list of clients
    struct BSSID* clients;
    //up to 255 clients per AP
    uint8_t num_clients;
    bool keyCaptured;
}AP;

#define PACK_BSSID(old_bssid, new_bssid) do{ \
    sscanf(old_bssid, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", \
        &(new_bssid)->oui[0],&(new_bssid)->oui[1],&(new_bssid)->oui[2],\
        &(new_bssid)->nic[0],&(new_bssid)->nic[1],&(new_bssid)->nic[2]);\
} while (0)

#define PRINT_BSSID(bssid) do{ \
    printf("%02X:%02X:%02X:%02X:%02X:%02X", \
        (bssid)->oui[0],(bssid)->oui[1],(bssid)->oui[2],\
        (bssid)->nic[0],(bssid)->nic[1],(bssid)->nic[2]); \
} while (0)

void initVals();
void readCSV(char* filename);
void read_Station_Data(char* line);
void read_AP_Data(char* line);
AP createAP(char* name, struct BSSID bssid);
void addClient(AP ap, struct BSSID bssid);
void destroyAP(AP ap);
char** getCompatibleChannels(int* num_channels);
void printReport();