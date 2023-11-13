#ifndef COLLECTION_H
#define COLLECTION_H

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <regex.h>
#include "common_dependancies.h"
#include "wifi.h"
#include "scanning_data.h"
#include "WPA_attack.h"

#define OUTPUT_FOLDER = "output/"
#define MAX_ATTACK = 5; //# of attacks/client
#define SCANNING_TIMEOUT = 10; //sec

void initVals();
void cleanup();
int startAirodumpScan(char* prefix, char* channel);
void stopAirodumpScan(int pid);
int attackAPs(int channel, char*);

#endif