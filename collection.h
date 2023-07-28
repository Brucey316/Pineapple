#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdbool.h>
#include <signal.h>
#include <regex.h>

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
    int attacked;
}AP;
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

void initVals();
void cleanup();
int startAirodumpScan(char* prefix, char* channel);
void stopAirodumpScan(int pid);
int attackAPs(int channel, char*);
void runDeauth(char* ap_bssid, char* client_bssid);
void checkKey(char* filename, int channel);
void readCSV(char* filename);
void read_Station_Data(char* line);
void read_AP_Data(char* line);
AP createAP(char* name, struct BSSID bssid);
void addClient(AP* ap, struct BSSID bssid);
void destroyAP(AP ap);
char* getWirelessDevice();
char** getCompatibleChannels(int* num_channels);
void printReport();