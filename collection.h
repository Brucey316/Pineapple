#ifndef COLLECTION_H
#define COLLECTION_H

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define msleep(x) Sleep(x)
#else 
    #include <unistd.h>
    #define msleep(x) usleep(x*1000)
#endif

#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <regex.h>
#include "common_dependancies.h"
#include "wifi.h"
#include "scanning_data.h"
#include "WPA_attack.h"
#include "screen.h"

#define OUTPUT_FOLDER "output/"
#define OUTPUT_FILE "results.pcap"
#define MAX_ATTACK 5 //# of attacks/client
#define SCANNING_TIMEOUT 10 //sec to wait without attacking any victims on channel
#define HADNSHAKE_SLEEP 3
#define POST_ATTACK_TIMEOUT 20 //sec to wait after attacking victim

void initVals();
void memory_cleanup();
void file_cleanup(char* cap, char* csv);
int startAirodumpScan(char* prefix, char* channel);
void stopAirodumpScan(int pid);
bool attackAPs(int channel, char*);

#endif